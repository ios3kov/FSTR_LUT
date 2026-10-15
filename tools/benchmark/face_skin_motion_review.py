"""Every-frame visual evidence and frame-aligned diagnostics, not playback verdict.

Reads exact final native float output; no new inference or output color changes.
The reviewer must inspect generated sheets and enter a separate actual verdict.
"""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from smart_analysis import encode_srgb


def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for part in iter(lambda:stream.read(1024*1024),b''):h.update(part)
    return h.hexdigest()


def main():
    p=argparse.ArgumentParser();p.add_argument('--job',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();report=json.loads((a.job/'verification.json').read_text());n,h,w=report['frame_count'],report['height'],report['width']
    if (n,h,w)!=(156,640,360):raise ValueError('Review is scoped to the existing 156-frame portrait trial')
    a.output.mkdir(parents=True,exist_ok=False)
    frames={key:np.memmap(a.job/(key+'.rgba32f'),dtype=np.float32,mode='r',shape=(n,h,w,4)) for key in ('input','balanced','spatial','smart')}
    mask=np.memmap(a.job/'stable-mask.f32',dtype=np.float32,mode='r',shape=(n,h,w))
    spatial=np.memmap(a.job/'spatial-mask.f32',dtype=np.float32,mode='r',shape=(n,h,w))
    font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',15)
    convert=lambda x:Image.fromarray(np.rint(np.clip(encode_srgb(x),0,1)*255).astype(np.uint8))
    coverage=[]
    # Full frame at native 360x640, every frame; six frames per sheet, paired result.
    for start in range(0,n,6):
        board=Image.new('RGB',(1440,3*680),(18,20,24));d=ImageDraw.Draw(board)
        for i in range(start,min(start+6,n)):
            cell=i-start;row,col=divmod(cell,2);x=col*720;y=row*680
            for j,key in enumerate(('input','smart')):
                d.text((x+j*360+8,y+7),f'{i:03d} / {i/25:.2f}s / {key}',font=font,fill='white')
                board.paste(convert(frames[key][i,...,:3]),(x+j*360,y+32))
        name=f'full-{start:03d}-{min(n-1,start+5):03d}.jpg';board.save(a.output/name,quality=95,subsampling=0)
        coverage.append(dict(file=name,frames=list(range(start,min(start+6,n))),sha256=sha(a.output/name)))
    # Manually source-anchored moving inspection crop, identical across variants.
    # Anchor interpolation does not use detector/mask positions and cannot
    # correct their jitter relative to source. Full-frame sheets remain above.
    anchors=np.array([[0,190,291],[50,195,294],[100,177,233],[130,177,212],
                      [140,177,165],[145,158,80],[149,150,35],[155,150,35]],float)
    cw,ch=192,192
    for start in range(0,n,8):
        board=Image.new('RGB',(cw*6,8*(ch+26)),(18,20,24));d=ImageDraw.Draw(board)
        for i in range(start,min(start+8,n)):
            y=(i-start)*(ch+26)
            cx=np.interp(i,anchors[:,0],anchors[:,1]);cy=np.interp(i,anchors[:,0],anchors[:,2])
            crop=(round(cx-48),round(cy-48),round(cx+48),round(cy+48))
            images=[convert(frames[key][i,...,:3]) for key in ('input','balanced','spatial','smart')]
            images += [Image.fromarray(np.rint(m[i]*255).astype(np.uint8)).convert('RGB') for m in (spatial,mask)]
            for j,(im,label) in enumerate(zip(images,('source','balance+LUT','spatial','temporal','spatial mask','temporal mask'))):
                d.text((j*cw+4,y+4),f'{i:03d} {label}',font=font,fill='white')
                board.paste(im.crop(crop).resize((cw,ch),Image.Resampling.NEAREST),(j*cw,y+26))
        name=f'detail-{start:03d}-{min(n-1,start+7):03d}.png';board.save(a.output/name)
        coverage.append(dict(file=name,frames=list(range(start,min(start+8,n))),sha256=sha(a.output/name)))
    # All-frame mask placement overview, including regions outside the face.
    for start in range(0,n,24):
        board=Image.new('RGB',(180*6,6*348),(18,20,24));d=ImageDraw.Draw(board)
        for i in range(start,min(start+24,n)):
            row,col=divmod(i-start,4);x=col*270;y=row*348
            rgb=np.clip(encode_srgb(frames['input'][i,...,:3]),0,1)
            overlay=rgb*(1-mask[i,...,None]*.7)+np.array([.15,.95,.5])*mask[i,...,None]*.7
            im=Image.fromarray(np.rint(overlay*255).astype(np.uint8)).resize((180,320),Image.Resampling.LANCZOS)
            d.text((x+5,y+5),f'{i:03d} / {i/25:.2f}s',font=font,fill='white');board.paste(im,(x,y+28))
        name=f'overview-{start:03d}-{min(n-1,start+23):03d}.png';board.save(a.output/name)
        coverage.append(dict(file=name,frames=list(range(start,min(start+24,n))),sha256=sha(a.output/name)))
    pixel_delta=np.max(np.abs(np.clip(encode_srgb(frames['smart'][...,:3]),0,1)-np.clip(encode_srgb(frames['balanced'][...,:3]),0,1)),axis=-1)
    active=np.any(mask>0,axis=(1,2));mean_mask=mask.mean((1,2))
    metrics=dict(zero_mask_frames=np.flatnonzero(~active).tolist(),
        on_off_transitions=[i for i in range(1,n) if active[i]!=active[i-1]],
        mean_mask=mean_mask.tolist(),effect_peak_srgb=pixel_delta.max((1,2)).tolist(),
        max_effect_jump_srgb=float(np.max(np.abs(np.diff(pixel_delta,axis=0)))),
        interpretation='on/off and delta jumps are defect locators, not motion-compensated flicker or visual verdict')
    (a.output/'coverage.json').write_text(json.dumps(dict(frame_count=n,fps=25,coverage=coverage,metrics=metrics,
        script_sha256=sha(Path(__file__)),source_verification_sha256=sha(a.job/'verification.json'),
        inputs={name:sha(a.job/name) for name in ('stable-mask.f32','spatial-mask.f32','input.rgba32f','spatial.rgba32f','smart.rgba32f','balanced.rgba32f')},
        visual_review_status='NOT_RUN until sheets actually inspected; normal-speed perceptual playback is separate'),indent=2)+'\n')
    print(json.dumps(metrics,indent=2))

if __name__=='__main__':main()
