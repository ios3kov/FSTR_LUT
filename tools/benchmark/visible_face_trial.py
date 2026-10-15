"""Offline occlusion ablation against an exact turn-recovery baseline."""
import argparse,json,subprocess,time
from pathlib import Path
import cv2,numpy as np
from PIL import Image,ImageDraw,ImageFont
from smart_analysis import encode_srgb,decode_srgb
from visible_face_analysis import VisibleFaceSegmenter,limit_mask
from turn_recovery_trial import sha


def validate_baseline(r):
    fields=('frame_count','width','height','fps','analysis_width','analysis_height','input_transfer')
    if tuple(r.get(k) for k in fields)!=(156,360,640,25,720,1280,'gamma24'):
        raise ValueError('Unsupported baseline geometry/transfer')


def main():
    p=argparse.ArgumentParser()
    for name in ('baseline','source','model','renderer','look','scenes','job','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();r=json.loads((a.baseline/'verification.json').read_text())
    validate_baseline(r)
    for name,key in [('source','source_sha256'),('renderer','renderer_sha256'),('look','look_sha256')]:
        if sha(getattr(a,name))!=r[key]:raise ValueError('Changed baseline '+name)
    a.job.mkdir(parents=True,exist_ok=False);a.output.mkdir(parents=True,exist_ok=False)
    snapshot=a.job/'source';snapshot.mkdir()
    for name in ('visible_face_analysis.py','visible_face_trial.py','smart_analysis.py','turn_recovery_trial.py'):(snapshot/name).write_bytes((Path(__file__).parent/name).read_bytes())
    sources={x.name:sha(x) for x in snapshot.iterdir()}
    import torch
    torch.set_num_threads(2);cv2.setNumThreads(1);model=VisibleFaceSegmenter(a.model)
    n,h,w=156,640,360
    oldmask=np.memmap(a.baseline/'stable-mask.f32',np.float32,'r',shape=(n,h,w))
    original=np.memmap(a.baseline/'input.rgba32f',np.float32,'r',shape=(n,h,w,4))
    old=np.memmap(a.baseline/'smart.rgba32f',np.float32,'r',shape=(n,h,w,4))
    masks=np.lib.format.open_memmap(a.job/'gated.npy',mode='w+',dtype=np.float32,shape=(n,h,w))
    timings=[];repeat={};counts=[]
    command=['ffmpeg','-v','error','-i',str(a.source),'-an','-vf','fps=25,scale=720:1280:flags=lanczos:in_color_matrix=bt709:out_range=full','-frames:v','156','-pix_fmt','rgb24','-f','rawvideo','pipe:1']
    with (a.job/'decoder.log').open('wb') as errors:
        proc=subprocess.Popen(command,stdout=subprocess.PIPE,stderr=errors)
        try:
            for i in range(n):
                raw=proc.stdout.read(h*w*12)
                if len(raw)!=h*w*12:raise ValueError('Incomplete source frame')
                rgb=encode_srgb((np.frombuffer(raw,np.uint8).reshape(h*2,w*2,3).astype(np.float32)/255)**2.4)
                boxes=[f['box'] for f in r['details'][i]['faces']]
                start=time.perf_counter();visibility=model.analyze(rgb,boxes,1.5);timings.append(1000*(time.perf_counter()-start))
                masks[i]=limit_mask(oldmask[i],cv2.resize(visibility,(w,h),interpolation=cv2.INTER_AREA))
                if i in (0,55,131,132,155):repeat[i]=(rgb.copy(),boxes,masks[i].copy())
                counts.append(dict(frame=i,old_mass=float(oldmask[i].sum()),new_mass=float(masks[i].sum())))
                if i%25==0:print('FRAME',i,flush=True)
            if proc.stdout.read(1) or proc.wait(timeout=30)!=0:raise RuntimeError('Decoder failure')
        finally:
            proc.stdout.close()
            if proc.poll() is None:proc.terminate()
            try:proc.wait(timeout=15)
            except subprocess.TimeoutExpired:proc.kill();proc.wait()
    for i in (155,132,0,131,55):
        rgb,boxes,expected=repeat[i]
        if i==55:rgb=rgb.astype(np.float64) # Public float64 input must normalize to float32.
        actual=limit_mask(oldmask[i],cv2.resize(model.analyze(rgb,boxes),(w,h),interpolation=cv2.INTER_AREA))
        if not np.array_equal(actual,expected):raise ValueError('Model repeat changed')
    masks.tofile(a.job/'gated-mask.f32')
    render=subprocess.run([str(a.renderer.resolve()),str(w),str(h),str((a.baseline/'input.rgba32f').resolve()),str((a.job/'gated-mask.f32').resolve()),str((a.baseline/'balance.f64').resolve()),str((a.job/'gated.rgba32f').resolve()),str(a.look.resolve()),'1','.8'],capture_output=True,text=True,check=True)
    result=np.memmap(a.job/'gated.rgba32f',np.float32,'r',shape=(n,h,w,4));balanced=np.memmap(a.baseline/'balanced.rgba32f',np.float32,'r',shape=result.shape)
    if not np.isfinite(result).all() or not np.all(result[...,3]==1):raise ValueError('Invalid render')
    background=bool(np.array_equal(result[masks==0],balanced[masks==0]));luma=float(np.abs((result[...,:3]-balanced[...,:3])@np.array([.2126,.7152,.0722])).max())
    if not background or luma>1e-6 or np.any(masks>oldmask):raise ValueError('Gate/render invariant')
    font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',14)
    u8=lambda x:np.rint(np.clip(x,0,1)*255).astype(np.uint8)
    overlay=lambda rgb,m:rgb*(1-.7*m[...,None])+np.array([.15,.95,.5])*.7*m[...,None]
    # Scene regression is repeated on final source, not inherited from the probe.
    scene_report=json.loads((a.scenes/'verification.json').read_text());sc=[];panels=[]
    for case in scene_report['cases']:
        path=a.scenes/(case['name']+'.npz');d=np.load(path);rgb=d['rgb'];before=d['mask']
        visibility=model.analyze(rgb,[f['box'] for f in case['candidate']['faces']]);after=limit_mask(before,visibility)
        np.savez_compressed(a.job/(case['name']+'.npz'),visibility=visibility,mask=after)
        sc.append(dict(name=case['name'],input_sha256=sha(path),before_mass=float(before.sum()),after_mass=float(after.sum())))
        panels.append((case['name'],rgb,overlay(rgb,before),overlay(rgb,after)))
    for start in range(0,len(panels),5):
        board=Image.new('RGB',(1152,190*len(panels[start:start+5])),(18,20,24));draw=ImageDraw.Draw(board)
        for row,entry in enumerate(panels[start:start+5]):
            for col,(label,im) in enumerate(zip(('source','previous mask','visible-face gate'),entry[1:])):
                draw.text((col*384+3,row*190+4),entry[0]+' / '+label,font=font,fill='white');board.paste(Image.fromarray(u8(im)).resize((384,160)),(col*384,row*190+28))
        board.save(a.output/f'scenes-{start:02d}.png')
    anchors=np.array([[0,190,291],[50,195,294],[100,177,233],[130,177,212],[140,177,165],[145,158,80],[149,150,35],[155,150,35]])
    coverage=[]
    for start in range(0,n,12):
        board=Image.new('RGB',(720,12*170),(18,20,24));draw=ImageDraw.Draw(board)
        for i in range(start,min(n,start+12)):
            rgb=np.clip(encode_srgb(original[i,...,:3]),0,1);cx=np.interp(i,anchors[:,0],anchors[:,1]);cy=np.interp(i,anchors[:,0],anchors[:,2]);crop=(round(cx-48),round(cy-48),round(cx+48),round(cy+48))
            ims=[rgb,overlay(rgb,oldmask[i]),overlay(rgb,masks[i]),encode_srgb(old[i,...,:3]),encode_srgb(result[i,...,:3])]
            for col,(label,im) in enumerate(zip(('source','old mask','new mask','old color','new color'),ims)):
                y=(i-start)*170;draw.text((col*144+2,y+3),f'{i:03d} '+label,font=font,fill='white');board.paste(Image.fromarray(u8(im)).crop(crop).resize((144,144),Image.Resampling.NEAREST),(col*144,y+26))
        name=f'detail-{start:03d}.png';board.save(a.output/name);coverage.append(dict(file=name,frames=list(range(start,min(n,start+12)))))
    video=a.output/'Occlusion-comparison.mp4'
    encoder=subprocess.Popen(['ffmpeg','-v','error','-f','rawvideo','-pix_fmt','rgb24','-s','1080x712','-r','25','-i','pipe:0','-an','-c:v','libx264','-crf','17','-pix_fmt','yuv420p','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-movflags','+faststart',str(video)],stdin=subprocess.PIPE,stderr=subprocess.PIPE)
    try:
        for i in range(n):
            board=Image.new('RGB',(1080,712),(18,20,24));draw=ImageDraw.Draw(board)
            for col,(label,frames) in enumerate(zip(('Source','Previous protection','Visible-face gate'),(original,old,result))):
                draw.text((col*w+8,12),f'{i:03d} / '+label,font=font,fill='white');board.paste(Image.fromarray(u8(encode_srgb(frames[i,...,:3]))),(col*w,72))
            if i in (55,130,131,132,133,139,140,141,142):board.save(a.output/f'full-{i:03d}.png')
            encoder.stdin.write(u8(decode_srgb(np.asarray(board).astype(np.float32)/255)**(1/2.4)).tobytes())
        encoder.stdin.close();errors=encoder.stderr.read()
        if encoder.wait(timeout=30):raise RuntimeError(errors.decode())
    finally:
        if encoder.poll() is None:encoder.terminate()
        try:encoder.wait(timeout=15)
        except subprocess.TimeoutExpired:encoder.kill();encoder.wait()
        encoder.stderr.close()
    if any(sha(Path(__file__).parent/k)!=v for k,v in sources.items()):raise RuntimeError('Changed executed code')
    report=dict(sources=sources,model_sha256=sha(a.model),source_sha256=r['source_sha256'],renderer_sha256=r['renderer_sha256'],look_sha256=r['look_sha256'],baseline_report_sha256=sha(a.baseline/'verification.json'),input_transfer='explicit gamma24 assumption',crop_factor=1.5,roll_hypotheses=[-30,0,30],threshold=.5,counts=counts,model_repeat='PASS155/132/0/131/55 exact',background_exact=background,luma_max_error=luma,zero_mask_frames=np.flatnonzero(~np.any(masks>0,axis=(1,2))).tolist(),native=json.loads(render.stdout),inference_ms=dict(median=float(np.median(timings)),p95=float(np.percentile(timings,95))),scene_cases=sc,coverage=coverage,artifacts={p.name:sha(p) for p in a.output.iterdir()},inputs={k:sha(a.baseline/k) for k in ('input.rgba32f','stable-mask.f32','smart.rgba32f','balance.f64','balanced.rgba32f')})
    (a.job/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print('COMPLETE',report['zero_mask_frames'],flush=True)
if __name__=='__main__':main()
