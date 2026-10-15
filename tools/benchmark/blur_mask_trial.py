"""Frozen-baseline blur experiment; no model inference, AE, or installation."""
import argparse,json,subprocess
from pathlib import Path
import cv2,numpy as np
from PIL import Image,ImageDraw
from blur_mask_recovery import recover_blur
from smart_analysis import encode_srgb
from turn_recovery_trial import sha
from visible_face_trial import validate_baseline

def main():
    p=argparse.ArgumentParser()
    for name in ('baseline','occlusion','scenes','renderer','look','job','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();report=json.loads((a.occlusion/'verification.json').read_text());oldreport=json.loads((a.baseline/'verification.json').read_text());validate_baseline(oldreport)
    if sha(a.baseline/'verification.json')!=report['baseline_report_sha256']:raise ValueError('Changed baseline')
    for name,h in report['inputs'].items():
        if sha(a.baseline/name)!=h:raise ValueError('Changed input '+name)
    for name in ('renderer','look'):
        if sha(getattr(a,name))!=report[name+'_sha256']:raise ValueError('Changed '+name)
    # Bind cached model output to the report's exact frame sums as well as its
    # hash in this run; source masks are recomputed nowhere in this experiment.
    gate=np.load(a.occlusion/'gated.npy',mmap_mode='r');n,h,w=156,640,360
    if gate.shape!=(n,h,w) or gate.dtype!=np.float32:raise ValueError('Cached gate shape')
    for i,c in enumerate(report['counts']):
        if not np.isclose(float(gate[i].sum()),c['new_mass'],rtol=2e-6,atol=1e-6):raise ValueError('Changed cached gate')
    raw_gate=np.memmap(a.occlusion/'gated-mask.f32',np.float32,'r',shape=gate.shape)
    if not np.array_equal(gate,raw_gate):raise ValueError('Cached mask formats disagree')
    a.job.mkdir(parents=True,exist_ok=False);a.output.mkdir(parents=True,exist_ok=False)
    source_names=('blur_mask_trial.py','blur_mask_recovery.py','turn_skin_recovery.py','smart_analysis.py')
    sources={name:sha(Path(__file__).parent/name) for name in source_names}
    raw=np.memmap(a.baseline/'input.rgba32f',np.float32,'r',shape=(n,h,w,4));support=np.memmap(a.baseline/'stable-mask.f32',np.float32,'r',shape=(n,h,w))
    images=[np.clip(encode_srgb(x[...,:3]),0,1) for x in raw];cuts=oldreport['cuts'];info=[]
    masks=np.lib.format.open_memmap(a.job/'masks.npy',mode='w+',dtype=np.float32,shape=gate.shape)
    for i in range(n):
        masks[i],d=recover_blur(images,support,gate,i,cuts);info.append(dict(frame=i,**d))
    for i in (155,55,0,130,37):
        again,_=recover_blur(images,support,gate,i,cuts)
        if not np.array_equal(again,masks[i]):raise ValueError('Order dependent result')
    if np.any(masks>support) or np.any(masks<gate):raise ValueError('Mask bound changed')
    scenes=[]
    for c in report['scene_cases']:
        original=a.scenes/(c['name']+'.npz')
        if sha(original)!=c['input_sha256']:raise ValueError('Changed scene')
        data=np.load(original);g=np.load(a.occlusion/(c['name']+'.npz'))['mask'];rgb=data['rgb'];b=data['mask']
        out,d=recover_blur([rgb]*3,[b]*3,[g]*3,1,[False,False])
        if not np.array_equal(out,g):raise ValueError('Static occluder reintroduced')
        scenes.append(dict(name=c['name'],unchanged=True,input_sha256=sha(original),gate_sha256=sha(a.occlusion/(c['name']+'.npz'))))
    masks.tofile(a.job/'mask.f32')
    native=subprocess.run([str(a.renderer.resolve()),str(w),str(h),str((a.baseline/'input.rgba32f').resolve()),str((a.job/'mask.f32').resolve()),str((a.baseline/'balance.f64').resolve()),str((a.job/'render.rgba32f').resolve()),str(a.look.resolve()),'1','.8'],capture_output=True,text=True,check=True)
    result=np.memmap(a.job/'render.rgba32f',np.float32,'r',shape=raw.shape);balanced=np.memmap(a.baseline/'balanced.rgba32f',np.float32,'r',shape=raw.shape);old=np.memmap(a.occlusion/'gated.rgba32f',np.float32,'r',shape=raw.shape)
    if not np.isfinite(result).all() or not np.all(result[...,3]==1):raise ValueError('Invalid native output')
    background=bool(np.array_equal(result[masks==0],balanced[masks==0]));luma=float(np.abs((result[...,:3]-balanced[...,:3])@np.array([.2126,.7152,.0722])).max())
    if not background or luma>1e-6:raise ValueError('Native invariant')
    u8=lambda x:np.rint(np.clip(x,0,1)*255).astype(np.uint8)
    overlay=lambda x,m:x*(1-.7*m[...,None])+np.array([.15,.95,.5])*.7*m[...,None]
    anchors=np.array([[0,190,291],[50,195,294],[100,177,233],[130,177,212],[140,177,165],[145,158,80],[149,150,35],[155,150,35]])
    for start in range(0,n,12):
        board=Image.new('RGB',(720,12*170),(18,20,24));draw=ImageDraw.Draw(board)
        for i in range(start,min(n,start+12)):
            cx=np.interp(i,anchors[:,0],anchors[:,1]);cy=np.interp(i,anchors[:,0],anchors[:,2]);box=(round(cx-48),round(cy-48),round(cx+48),round(cy+48))
            ims=[images[i],overlay(images[i],gate[i]),overlay(images[i],masks[i]),encode_srgb(old[i,...,:3]),encode_srgb(result[i,...,:3])]
            for col,(label,im) in enumerate(zip(('source','old mask','recovery','old color','new color'),ims)):
                draw.text((col*144+2,(i-start)*170+2),f'{i:03d} {label}',fill='white');board.paste(Image.fromarray(u8(im)).crop(box).resize((144,144)),(col*144,(i-start)*170+24))
        board.save(a.output/f'detail-{start:03d}.png')
    video=a.output/'Blur-recovery-comparison.mp4'
    proc=subprocess.Popen(['ffmpeg','-v','error','-f','rawvideo','-pix_fmt','rgb24','-s','1080x680','-r','25','-i','pipe:0','-an','-c:v','libx264','-crf','18','-pix_fmt','yuv420p',str(video)],stdin=subprocess.PIPE,stderr=subprocess.PIPE)
    try:
        for i in range(n):
            frame=Image.new('RGB',(1080,680),(18,20,24));draw=ImageDraw.Draw(frame)
            for col,(label,im) in enumerate(zip(('Source','Visible-face gate','Blur recovery'),(images[i],encode_srgb(old[i,...,:3]),encode_srgb(result[i,...,:3])))):
                draw.text((col*360+8,12),f'{i:03d} / {label}',fill='white');frame.paste(Image.fromarray(u8(im)),(col*360,40))
            if i in (37,54,55,56,130,140):frame.save(a.output/f'full-{i:03d}.png')
            proc.stdin.write(np.array(frame).tobytes())
        proc.stdin.close();err=proc.stderr.read();code=proc.wait(timeout=30)
        if code:raise RuntimeError(err.decode())
    finally:
        if proc.poll() is None:proc.kill();proc.wait()
        proc.stderr.close()
    for name,digest in sources.items():
        if sha(Path(__file__).parent/name)!=digest:raise ValueError('Source changed during run')
    result_report=dict(sources=sources,baseline_sha256=sha(a.baseline/'verification.json'),occlusion_sha256=sha(a.occlusion/'verification.json'),cached_gate_sha256=sha(a.occlusion/'gated.npy'),mask_sha256=sha(a.job/'mask.f32'),render_sha256=sha(a.job/'render.rgba32f'),renderer_sha256=sha(a.renderer),look_sha256=sha(a.look),frames=info,scenes=scenes,background_exact=background,luma_max_error=luma,model_inference='NOT RUN: frozen validated baseline',native=native.stdout,nonchronological_repeat='PASS',artifacts={f.name:sha(f) for f in a.output.iterdir()})
    (a.job/'verification.json').write_text(json.dumps(result_report,indent=2)+'\n');print(json.dumps([d for d in info if d['status']=='recovered'],indent=2))
if __name__=='__main__':main()
