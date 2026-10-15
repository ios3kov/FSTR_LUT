"""Exact-baseline private turn-recovery trial; no AE or production claims."""
import argparse,hashlib,json,os,subprocess,time
from pathlib import Path
import cv2,numpy as np
from PIL import Image,ImageDraw,ImageFont
from smart_analysis import encode_srgb,decode_srgb,cut_between,stabilize_mask
from face_skin_analysis import FaceSkinSegmenter,stabilize_face_mask
from turn_skin_recovery import TurnSkinSegmenter,combine_turn_window


def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
    return h.hexdigest()


def main():
    p=argparse.ArgumentParser()
    for name in ('baseline','source','model','face-model','detector-model','renderer','look','job','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args()
    for name in ('baseline','source','model','face_model','detector_model','renderer','look'):setattr(a,name,getattr(a,name).resolve(strict=True))
    r=json.loads((a.baseline/'verification.json').read_text())
    if (r['frame_count'],r['width'],r['height'],r['input_transfer'])!=(156,360,640,'gamma24'):raise ValueError('Unsupported baseline')
    for name,key in [('source','source_sha256'),('renderer','renderer_sha256'),('look','look_sha256')]:
        if sha(getattr(a,name))!=r[key]:raise ValueError('Changed baseline '+name)
    a.job.mkdir(parents=True,exist_ok=False);a.output.mkdir(parents=True,exist_ok=False)
    src=a.job/'source';src.mkdir()
    for name in ('turn_recovery_trial.py','turn_skin_recovery.py','face_skin_analysis.py','smart_analysis.py'):(src/name).write_bytes((Path(__file__).parent/name).read_bytes())
    sources={x.name:sha(x) for x in src.iterdir()}
    os.environ.setdefault('MPLCONFIGDIR',str(a.job/'mpl'));cv2.setNumThreads(1)
    n,h,w=156,640,360
    for name in ('input.rgba32f','balance.f64','identity.f64','balanced.rgba32f','lut.rgba32f','prepared.rgba32f'):
        # Immutable baseline links; never open these paths for writing.
        os.link(a.baseline/name,a.job/name)
    original=np.memmap(a.job/'input.rgba32f',np.float32,'r',shape=(n,h,w,4));images=encode_srgb(original[...,:3]).astype(np.float32)
    arrays={key:np.lib.format.open_memmap(a.job/(key+'.npy'),mode='w+',dtype=np.float32,shape=(n,h,w)) for key in ('mesh','support','proposal','semantic','profile')}
    records=[];timings=[];repeat={}
    command=['ffmpeg','-v','error','-i',str(a.source),'-an','-vf','fps=25,scale=720:1280:flags=lanczos:in_color_matrix=bt709:out_range=full','-frames:v','156','-pix_fmt','rgb24','-f','rawvideo','pipe:1']
    model=TurnSkinSegmenter(a.model,a.face_model,a.detector_model)
    process=None
    try:
        with (a.job/'decode.log').open('wb') as errors:
            try:
                process=subprocess.Popen(command,stdout=subprocess.PIPE,stderr=errors)
                for i in range(n):
                    raw=process.stdout.read(h*w*12)
                    if len(raw)!=h*w*12:raise ValueError('Incomplete analysis frame')
                    rgb=encode_srgb((np.frombuffer(raw,np.uint8).reshape(h*2,w*2,3).astype(np.float32)/255)**2.4)
                    if i in (0,55,78,155):repeat[i]=rgb.copy()
                    start=time.perf_counter();result=model.analyze_turn(rgb);timings.append((time.perf_counter()-start)*1000)
                    records.append(result.pop('detail'))
                    for key,values in arrays.items():values[i]=cv2.resize(result[key],(w,h),interpolation=cv2.INTER_AREA)
                    if i%25==0:print('ANALYSIS',i,records[-1]['mesh_faces'],records[-1]['viable_proposals'],flush=True)
                if process.stdout.read(1) or process.wait(timeout=30)!=0:raise RuntimeError('Decoder did not finish successfully')
            finally:
                if process is not None:
                    process.stdout.close()
                    if process.poll() is None:process.terminate()
                    try:process.wait(timeout=15)
                    except subprocess.TimeoutExpired:process.kill();process.wait()
        repeats=[]
        for i in (155,0,78):
            again=model.analyze_turn(repeat[i]);error=max(float(np.max(np.abs(cv2.resize(again[k],(w,h),interpolation=cv2.INTER_AREA)-arrays[k][i]))) for k in arrays)
            if error!=0:raise ValueError('Actual model order dependency')
            repeats.append(dict(frame=i,max_error=error))
    finally:model.close()
    # Verify the old default path remains pixel-identical after optional API additions.
    baseline_model=FaceSkinSegmenter(a.model,a.face_model,a.detector_model);baseline_checks=[]
    baseline_masks=np.load(a.baseline/'spatial-masks.npy',mmap_mode='r')
    try:
        for i in (0,55,78,155):
            m,_,_=baseline_model.analyze_with_support(repeat[i]);error=float(np.abs(cv2.resize(m,(w,h),interpolation=cv2.INTER_AREA)-baseline_masks[i]).max())
            if error!=0:raise ValueError('Default face mask changed')
            baseline_checks.append(dict(frame=i,max_error=error))
    finally:baseline_model.close()
    cuts=[cut_between(images[i-1],images[i]) for i in range(1,n)]
    counts=[d['mesh_faces'] for d in records];proposals=[d['viable_proposals'] for d in records]
    raw_spatial=np.maximum(arrays['mesh'],arrays['profile'])
    recovered=[];mesh_stable=[];profile_stable=[];provenance=[];temporal_ms=[]
    for i in range(n):
        start=time.perf_counter()
        result,info=combine_turn_window(images,arrays['mesh'],arrays['support'],arrays['profile'],arrays['proposal'],arrays['semantic'],counts,proposals,i,cuts)
        recovered.append(result['recovery_only']);provenance.append(info)
        mesh_stable.append(result['mesh']);profile_stable.append(result['spatial'])
        temporal_ms.append((time.perf_counter()-start)*1000)
        if i%25==0:print('TEMPORAL',i,info,flush=True)
    recovered=np.asarray(recovered);mesh_stable=np.asarray(mesh_stable);profile_stable=np.asarray(profile_stable)
    for i in (155,55,41,0,132,78):
        again,_=combine_turn_window(images,arrays['mesh'],arrays['support'],arrays['profile'],arrays['proposal'],arrays['semantic'],counts,proposals,i,cuts)
        if not np.array_equal(again['recovery_only'],recovered[i]) or not np.array_equal(again['spatial'],profile_stable[i]) or not np.array_equal(again['mesh'],mesh_stable[i]):raise ValueError('Temporal order dependency')
    masks={'mesh':mesh_stable,'spatial':profile_stable,'recovered':np.maximum(mesh_stable,recovered),'smart':np.maximum(profile_stable,recovered)}
    outputs={};native={}
    for key,mask in masks.items():
        mask.tofile(a.job/(key+'-mask.f32'));target=a.job/(key+'.rgba32f')
        run=subprocess.run([str(a.renderer),str(w),str(h),str(a.job/'input.rgba32f'),str(a.job/(key+'-mask.f32')),str(a.job/'balance.f64'),str(target),str(a.look),'1','.8'],capture_output=True,text=True,check=True)
        native[key]=json.loads(run.stdout);outputs[key]=np.memmap(target,np.float32,'r',shape=(n,h,w,4))
        if not np.isfinite(outputs[key]).all() or not np.all(outputs[key][...,3]==1):raise ValueError('Invalid native output')
        print('NATIVE',key,run.stdout.strip(),flush=True)
    masks['smart'].tofile(a.job/'stable-mask.f32')
    np.save(a.job/'spatial-masks.npy',raw_spatial);np.save(a.job/'recovery-only.npy',recovered)
    old=np.memmap(a.baseline/'smart.rgba32f',np.float32,'r',shape=(n,h,w,4));balanced=np.memmap(a.job/'balanced.rgba32f',np.float32,'r',shape=(n,h,w,4))
    font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',18)
    to_u8=lambda x:np.rint(np.clip(encode_srgb(x),0,1)*255).astype(np.uint8)
    for title,keys in [('Turn-recovery-comparison.mp4',('old','mesh','spatial','smart')),('Recovery-only-comparison.mp4',('old','mesh','recovered','smart'))]:
        cmd=['ffmpeg','-v','error','-f','rawvideo','-pix_fmt','rgb24','-s','1440x712','-r','25','-i','pipe:0','-an','-c:v','libx264','-crf','17','-pix_fmt','yuv420p','-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-movflags','+faststart',str(a.output/title)]
        with subprocess.Popen(cmd,stdin=subprocess.PIPE,stderr=subprocess.PIPE) as proc:
            for i in range(n):
                board=Image.new('RGB',(1440,712),(18,20,24));draw=ImageDraw.Draw(board)
                for k,key in enumerate(keys):
                    values=old if key=='old' else outputs[key]
                    label={'old':'Previous face protection','mesh':'More sensitive + mesh','spatial':'Mesh + profile fallback','recovered':'Mesh + short-gap recovery','smart':'Combined candidate'}[key]
                    draw.text((k*w+8,10),label,font=font,fill='white');draw.text((k*w+8,38),f'{i:03d} / {i/25:.2f}s / research',font=font,fill='white');board.paste(Image.fromarray(to_u8(values[i,...,:3])),(k*w,72))
                if i in (37,40,41,55,131,132,140,141,146) and title.startswith('Turn-'):board.save(a.output/f'Turn-{i:03d}.png')
                gamma=np.rint(decode_srgb(np.asarray(board).astype(np.float32)/255)**(1/2.4)*255).astype(np.uint8)
                proc.stdin.write(gamma.tobytes())
            proc.stdin.close();errors=proc.stderr.read()
            if proc.wait():raise RuntimeError(errors.decode())
    metrics={}
    for key,mask in masks.items():
        delta=outputs[key][...,:3]-balanced[...,:3]
        metrics[key]=dict(zero_mask_frames=np.flatnonzero(~np.any(mask>0,axis=(1,2))).tolist(),background_exact=bool(np.array_equal(outputs[key][...,:3][mask==0],balanced[...,:3][mask==0])),luma_max_error=float(np.abs(delta@np.array([.2126,.7152,.0722])).max()))
        if not metrics[key]['background_exact'] or metrics[key]['luma_max_error']>1e-6:raise ValueError('Native invariant failure')
    if any(sha(Path(__file__).parent/name)!=digest for name,digest in sources.items()):raise RuntimeError('Source changed during trial')
    report=dict(frame_count=n,width=w,height=h,fps=25,input_transfer='gamma24',analysis_width=720,analysis_height=1280,code_sha256=sources,baseline_verification_sha256=sha(a.baseline/'verification.json'),source_sha256=r['source_sha256'],renderer_sha256=sha(a.renderer),look_sha256=sha(a.look),models={name:sha(getattr(a,name)) for name in ('model','face_model','detector_model')},metrics=metrics,model_repeats=repeats,old_default_checks=baseline_checks,cuts=cuts,details=records,recovery=provenance,inference_ms=dict(median=float(np.median(timings)),p95=float(np.percentile(timings,95))),temporal_ms=dict(median=float(np.median(temporal_ms)),p95=float(np.percentile(temporal_ms,95))),native_timings=native,artifacts={x.name:sha(x) for x in a.output.iterdir() if x.is_file()})
    (a.job/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(metrics=metrics,recovered_frames=[i for i,d in enumerate(provenance) if d['status']=='temporal_recovery']),indent=2))

if __name__=='__main__':main()
