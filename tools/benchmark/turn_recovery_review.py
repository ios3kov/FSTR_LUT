"""Generate every-frame comparison sheets; actual inspection is recorded separately."""
import argparse,json
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from turn_recovery_trial import sha
from smart_analysis import encode_srgb


def main():
    p=argparse.ArgumentParser()
    for name in ('job','baseline','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();r=json.loads((a.job/'verification.json').read_text());n,h,w=r['frame_count'],r['height'],r['width']
    if (n,h,w)!=(156,640,360):raise ValueError('Unsupported trial')
    a.output.mkdir(parents=True,exist_ok=False);shape=(n,h,w,4)
    frames={key:np.memmap(a.job/(key+'.rgba32f'),np.float32,'r',shape=shape) for key in ('input','mesh','spatial','recovered','smart')}
    frames['old']=np.memmap(a.baseline/'smart.rgba32f',np.float32,'r',shape=shape)
    masks=[np.memmap(folder/'stable-mask.f32',np.float32,'r',shape=(n,h,w)) for folder in (a.baseline,a.job)]
    font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',14)
    convert=lambda x:Image.fromarray(np.rint(np.clip(encode_srgb(x),0,1)*255).astype(np.uint8))
    anchors=np.array([[0,190,291],[50,195,294],[100,177,233],[130,177,212],[140,177,165],[145,158,80],[149,150,35],[155,150,35]],float)
    coverage=[]
    def save(board,name,indices):
        board.save(a.output/name);coverage.append(dict(file=name,frames=list(indices),sha256=sha(a.output/name)))
    for start in range(0,n,8):
        board=Image.new('RGB',(1152,8*218),(18,20,24));d=ImageDraw.Draw(board)
        for i in range(start,min(start+8,n)):
            y=(i-start)*218;cx=np.interp(i,anchors[:,0],anchors[:,1]);cy=np.interp(i,anchors[:,0],anchors[:,2]);crop=(round(cx-48),round(cy-48),round(cx+48),round(cy+48))
            for k,(key,label) in enumerate(zip(('input','old','mesh','spatial','recovered','smart'),('source','previous','new mesh','mesh+profile','mesh+recovery','combined'))):
                d.text((k*192+3,y+4),f'{i:03d} {label}',font=font,fill='white');board.paste(convert(frames[key][i,...,:3]).crop(crop).resize((192,192),Image.Resampling.NEAREST),(k*192,y+26))
        save(board,f'detail-{start:03d}-{min(n-1,start+7):03d}.png',range(start,min(start+8,n)))
    for start in range(0,n,24):
        board=Image.new('RGB',(1440,6*348),(18,20,24));d=ImageDraw.Draw(board)
        for i in range(start,min(start+24,n)):
            row,col=divmod(i-start,4);x=col*360;y=row*348
            rgb=np.clip(encode_srgb(frames['input'][i,...,:3]),0,1)
            for k,(m,label) in enumerate(zip(masks,('old','new'))):
                overlay=rgb*(1-.7*m[i,...,None])+np.array([.15,.95,.5])*.7*m[i,...,None]
                im=Image.fromarray(np.rint(overlay*255).astype(np.uint8)).resize((180,320),Image.Resampling.LANCZOS)
                d.text((x+k*180+4,y+4),f'{i:03d} {label}',font=font,fill='white');board.paste(im,(x+k*180,y+28))
        save(board,f'masks-{start:03d}-{min(n-1,start+23):03d}.png',range(start,min(start+24,n)))
    # Additional whole-frame panels for transitions/occlusion and known false candidate.
    for i in (37,38,39,40,41,42,54,55,56,130,131,132,133,139,140,141,142,145,146,147):
        board=Image.new('RGB',(1080,676),(18,20,24));d=ImageDraw.Draw(board)
        for k,key in enumerate(('input','old','smart')):
            d.text((k*w+6,8),f'{i:03d} / {key}',font=font,fill='white');board.paste(convert(frames[key][i,...,:3]),(k*w,32))
        save(board,f'full-{i:03d}.png',[i])
    inputs={'job/'+key+'.rgba32f':sha(a.job/(key+'.rgba32f')) for key in ('input','mesh','spatial','recovered','smart')}
    inputs.update({'baseline/smart.rgba32f':sha(a.baseline/'smart.rgba32f'),'baseline/stable-mask.f32':sha(a.baseline/'stable-mask.f32'),'job/stable-mask.f32':sha(a.job/'stable-mask.f32')})
    (a.output/'coverage.json').write_text(json.dumps(dict(coverage=coverage,inputs=inputs,script_sha256=sha(Path(__file__)),verification_sha256=sha(a.job/'verification.json'),status='NOT RUN until actually inspected; normal-speed playback is separate'),indent=2)+'\n')

if __name__=='__main__':main()
