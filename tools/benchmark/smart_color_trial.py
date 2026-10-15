"""Reproducible private SDR trial. Outputs/footage/models must stay outside Git.

No AE API, no network, no implicit camera profile. Fresh output paths required.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from smart_analysis import (SkinSegmenter, MODEL_SHA, decode_srgb, encode_srgb,
                           estimate_balance, cut_between, stabilize_balance, stabilize_mask)


def sha(path):
    h=hashlib.sha256()
    with open(path,'rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
    return h.hexdigest()


def main():
    parser=argparse.ArgumentParser()
    for arg in ('source','model','renderer','look','job','output'):
        parser.add_argument('--'+arg,type=Path,required=True)
    parser.add_argument('--input-transfer',choices=('gamma24','srgb'),required=True)
    args=parser.parse_args()
    for name in ('source','model','renderer','look'):
        path=getattr(args,name).resolve(strict=True)
        if not path.is_file():raise ValueError('Expected input file: '+name)
        setattr(args,name,path)
    if args.look.name!='teal-orange-v1.cube':
        raise ValueError('This comparison trial is explicitly Teal & Orange; use a separate labeled trial for another look')
    args.job.mkdir(parents=True,exist_ok=False);args.output.mkdir(parents=True,exist_ok=False)
    snapshot=args.job/'source';snapshot.mkdir()
    for name in ('smart_color_trial.py','smart_analysis.py'):
        path=Path(__file__).parent/name
        (snapshot/name).write_bytes(path.read_bytes())
    code_sha256={p.name:sha(p) for p in snapshot.iterdir()}
    os.environ.setdefault('MPLCONFIGDIR',str(args.job/'mpl'))
    cv2.setNumThreads(1)
    w,h,fps=360,640,25
    # FFmpeg honors the source rotation. Decode range/matrix is explicit;
    # transfer is deliberately a required caller decision, not inferred Log.
    cmd=['ffmpeg','-v','error','-i',str(args.source),'-an','-vf',
         f'fps={fps},scale={w}:{h}:flags=lanczos:in_color_matrix=bt709:out_range=full',
         '-frames:v','156','-pix_fmt','rgb24','-f','rawvideo','pipe:1']
    decoded=subprocess.run(cmd,check=True,capture_output=True).stdout
    if not decoded or len(decoded)%(w*h*3):raise ValueError('Invalid decoded stream')
    u8=np.frombuffer(decoded,np.uint8).reshape(-1,h,w,3)
    count=len(u8)
    encoded=u8.astype(np.float32)/255
    linear=encoded**2.4 if args.input_transfer=='gamma24' else decode_srgb(encoded)
    images=encode_srgb(linear).astype(np.float32)
    rgba=np.ones((count,h,w,4),np.float32);rgba[...,:3]=linear
    rgba.tofile(args.job/'input.rgba32f');del rgba,encoded,decoded
    print(f'DECODE {count} frames {w}x{h} at{fps}fps; explicit {args.input_transfer}',flush=True)
    model=SkinSegmenter(args.model)
    masks=[];details=[];balances=[];inference_ms=[]
    try:
        for i,rgb in enumerate(images):
            start=time.perf_counter();mask,detail=model.analyze(rgb)
            inference_ms.append((time.perf_counter()-start)*1000)
            balance,bdetail=estimate_balance(rgb)
            masks.append(mask);balances.append(balance);details.append(dict(segmentation=detail,balance=bdetail))
            if i%25==0:print(f'ANALYSIS {i+1}/{count}; skin_area={mask.mean():.4f}',flush=True)
        # Same actual model in deliberately nonchronological IMAGE requests.
        repeated=[]
        for i in [count-1,0,count//2]:
            again,_=model.analyze(images[i]);error=float(np.abs(again-masks[i]).max())
            if error>1e-6:raise ValueError('Model inference depends on call order')
            repeated.append(dict(frame=i,max_error=error))
    finally:model.close()
    masks=np.asarray(masks);balances=np.asarray(balances)
    np.save(args.job/'spatial-masks.npy',masks);np.save(args.job/'balance-raw.npy',balances)
    (args.job/'analysis.json').write_text(json.dumps(dict(details=details,inference_ms=inference_ms,repeated=repeated),indent=2)+'\n')
    cuts=[cut_between(images[i-1],images[i]) for i in range(1,count)]
    stable=[];stabilized=[];temporal_ms=[]
    for i in range(count):
        start=time.perf_counter()
        stable.append(stabilize_mask(images,masks,i,cuts));stabilized.append(stabilize_balance(balances,i,cuts))
        temporal_ms.append((time.perf_counter()-start)*1000)
        if i%25==0:print(f'TEMPORAL {i+1}/{count}',flush=True)
    stable=np.asarray(stable);stabilized=np.asarray(stabilized)
    for i in [count-1,0,count//2]:
        if not np.array_equal(stable[i],stabilize_mask(images,masks,i,cuts)):raise ValueError('Temporal order dependency')
    stable.tofile(args.job/'stable-mask.f32');masks.tofile(args.job/'spatial-mask.f32')
    stabilized.astype(np.float64).tofile(args.job/'balance.f64')
    identity=np.tile([0.,1.,1.,1.],(count,1));identity.tofile(args.job/'identity.f64')
    timings={};outputs={}
    variants=[('lut','identity.f64','none',0,args.look),
              ('balanced','balance.f64','none',0,args.look),
              ('smart','balance.f64','stable-mask.f32',.8,args.look),
              ('spatial','balance.f64','spatial-mask.f32',.8,args.look),
              ('prepared','balance.f64','none',0,'none')]
    for name,balance,mask,protection,look in variants:
        target=args.job/(name+'.rgba32f')
        run=subprocess.run([str(args.renderer),str(w),str(h),str(args.job/'input.rgba32f'),
            str(args.job/mask) if mask!='none' else 'none',str(args.job/balance),str(target),
            str(look),'1',str(protection)],capture_output=True,text=True,check=True)
        timings[name]=json.loads(run.stdout)
        outputs[name]=np.memmap(target,dtype=np.float32,mode='r',shape=(count,h,w,4))
        if not np.isfinite(outputs[name]).all() or not np.all(outputs[name][...,3]==1):raise ValueError('Invalid native output/alpha')
        print('NATIVE',name,run.stdout.strip(),flush=True)
    font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',20)
    small=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',16)
    to_u8=lambda a:np.rint(np.clip(encode_srgb(a),0,1)*255).astype(np.uint8)
    def panels(i):
        source=np.rint(np.clip(images[i],0,1)*255).astype(np.uint8)
        overlay=source.astype(float);p=stable[i][...,None]*.65
        overlay=np.rint(overlay*(1-p)+np.array([40,240,130])*p).astype(np.uint8)
        return [source,to_u8(outputs['lut'][i,...,:3]),to_u8(outputs['smart'][i,...,:3]),overlay]
    titles=['Original','Teal & Orange / LUT','Smart / Skin Protection 80%','Skin mask / green overlay']
    for sec in (1,2,4):
        i=min(count-1,sec*fps);board=Image.new('RGB',(w*4,h+72),(19,21,25));draw=ImageDraw.Draw(board)
        for k,arr in enumerate(panels(i)):
            draw.text((k*w+12,10),titles[k],font=font,fill='white')
            draw.text((k*w+12,39),f'{sec}s / independent prototype',font=small,fill=(176,184,193))
            board.paste(Image.fromarray(arr),(k*w,72))
        board.save(args.output/f'Comparison-{sec}s.png')
    i=min(count-1,2*fps)
    crop=(145,260,250,450);board=Image.new('RGB',(420*4,820),(19,21,25));draw=ImageDraw.Draw(board)
    for k,arr in enumerate(panels(i)):
        draw.text((420*k+12,12),titles[k],font=font,fill='white')
        board.paste(Image.fromarray(arr).crop(crop).resize((420,760),Image.Resampling.LANCZOS),(420*k,60))
    board.save(args.output/'Portrait-detail.png')
    # Separate balance and temporal inspection; do not conflate their effects.
    for suffix,names in [('Balance',['source','prepared','lut','balanced']),
                         ('Temporal',['balanced','spatial','smart','mask'])]:
        board=Image.new('RGB',(w*4,h+72),(19,21,25));draw=ImageDraw.Draw(board)
        for k,name in enumerate(names):
            if name=='source':arr=np.rint(images[i]*255).astype(np.uint8)
            elif name=='mask':arr=np.repeat(np.rint(stable[i]*255).astype(np.uint8)[...,None],3,-1)
            else:arr=to_u8(outputs[name][i,...,:3])
            draw.text((k*w+12,12),name,font=font,fill='white');board.paste(Image.fromarray(arr),(k*w,72))
        board.save(args.output/(suffix+'-2s.png'))
    # Video preview uses Gamma2.4 display encoding and standard BT709 YUV tags.
    # Each panel's linear pixels follow the same transfer, including mask display.
    for video_name,mode in [('Smart-Color-comparison.mp4','main'),('Temporal-comparison.mp4','temporal')]:
        command=['ffmpeg','-v','error','-f','rawvideo','-pix_fmt','rgb24','-s',f'{w*4}x{h+72}',
                 '-r',str(fps),'-i','pipe:0','-an','-c:v','libx264','-crf','17','-pix_fmt','yuv420p',
                 '-color_primaries','bt709','-color_trc','bt709','-colorspace','bt709','-movflags','+faststart',str(args.output/video_name)]
        with subprocess.Popen(command,stdin=subprocess.PIPE,stderr=subprocess.PIPE) as process:
            for i in range(count):
                board=Image.new('RGB',(w*4,h+72),(19,21,25));draw=ImageDraw.Draw(board)
                if mode=='main':
                    arrays=panels(i);labels=titles
                else:
                    arrays=[to_u8(outputs[n][i,...,:3]) for n in ('lut','balanced','spatial','smart')]
                    labels=['LUT only','Balance + LUT','Spatial skin protection','Temporal skin protection']
                for k,arr in enumerate(arrays):
                    draw.text((k*w+12,10),labels[k],font=font,fill='white')
                    draw.text((k*w+12,39),f'{i/fps:.2f}s / prototype / 25 fps',font=small,fill=(176,184,193))
                    board.paste(Image.fromarray(arr),(k*w,72))
                srgb=np.asarray(board).astype(np.float32)/255
                gamma=np.rint(np.clip(decode_srgb(srgb),0,1)**(1/2.4)*255).astype(np.uint8)
                process.stdin.write(gamma.tobytes())
            process.stdin.close();errors=process.stderr.read();returncode=process.wait()
            if returncode:raise RuntimeError(errors.decode())
    # Measurement over declared semantic weights; not independent mask accuracy.
    rgb_smart=outputs['smart'][...,:3];rgb_bal=outputs['balanced'][...,:3];prepared=outputs['prepared'][...,:3]
    selected=stable>.8;background=stable==0
    metrics={
        'skin_weighted_selected_pixels':int(selected.sum()),
        'background_exact_equal':bool(np.array_equal(rgb_smart[background],rgb_bal[background])),
        'luminance_max_error':float(np.abs((rgb_smart-rgb_bal)@np.array([.2126,.7152,.0722])).max()),
        'smart_display_out_of_range_fraction':float(((rgb_smart<0)|(rgb_smart>1)).any(-1).mean()),
        'lut_display_out_of_range_fraction':float(((outputs['lut'][...,:3]<0)|(outputs['lut'][...,:3]>1)).any(-1).mean()),
        'skin_change_linear_mean':float(np.abs(rgb_smart[selected]-rgb_bal[selected]).mean()) if selected.any() else None,
        'exposure_minmax': [float(stabilized[:,0].min()),float(stabilized[:,0].max())],
        'gain_minmax':[float(stabilized[:,1:].min()),float(stabilized[:,1:].max())],
        'max_exposure_step':float(np.abs(np.diff(stabilized[:,0])).max()) if count>1 else 0.,
        'cut_indices':[i+1 for i,c in enumerate(cuts) if c],
        'model_repeated_reverse_requests':repeated,
        'raw_mask_adjacent_mae':float(np.abs(np.diff(masks,axis=0)).mean()) if count>1 else 0.,
        'stable_mask_adjacent_mae':float(np.abs(np.diff(stable,axis=0)).mean()) if count>1 else 0.,
    }
    # Adjacent MAE includes actual motion. No automatic temporal-quality PASS.
    report=dict(scope='Independent opaque SDR prototype; not AE render or artistic approval',
                frame_count=count,width=w,height=h,fps=fps,input_transfer=args.input_transfer,
                model_sha256=MODEL_SHA,source_sha256=sha(args.source),look_sha256=sha(args.look),code_sha256=code_sha256,
                renderer_sha256=sha(args.renderer),metrics=metrics,native_timings=timings,
                inference_ms=dict(median=float(np.median(inference_ms)),p95=float(np.percentile(inference_ms,95))),
                temporal_ms=dict(median=float(np.median(temporal_ms)),p95=float(np.percentile(temporal_ms,95))),
                details=details,artifacts={p.name:sha(p) for p in args.output.iterdir() if p.is_file()})
    (args.job/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
    (args.output/'README-RU.md').write_text('Исследовательский Smart Color, не новая сборка AE.\n\n'
        'Сравнение: исходник, обычный Teal & Orange 100%, умеренная подготовка + защита кожи80%, маска.\n'
        f'{count} кадров при{fps}fps,{w}×{h}; вход явно принят {args.input_transfer} SDR.\n'
        'Зелёная маска — результат модели, не ручная эталонная разметка. Качество на других людях/свете не доказано.\n'
        'Модель и видео обрабатываются локально. Установленный плагин не изменён.\n')
    print(json.dumps(dict(metrics=metrics,inference_ms=report['inference_ms'],temporal_ms=report['temporal_ms']),indent=2),flush=True)


if __name__=='__main__':main()
