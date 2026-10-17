"""Motion diagnostics, not a substitute for continuous perceptual acceptance."""
import argparse,json
from pathlib import Path
import cv2
import numpy as np
from PIL import Image,ImageDraw
p=argparse.ArgumentParser();p.add_argument('images',type=Path);p.add_argument('trial',type=Path);p.add_argument('--pattern',default='motion-*.png');p.add_argument('--fps',type=float,default=25);a=p.parse_args()
if not np.isfinite(a.fps) or not 0<a.fps<=240:raise ValueError('Invalid review frame rate')
frames=sorted(a.images.glob(a.pattern));pairs=[];prior=None
if len(frames)<2:raise ValueError('At least two frames required')
for path in frames:
 name=path.stem;rgb=np.asarray(Image.open(path),dtype=np.float32)/255;h,w=rgb.shape[:2]
 raw=np.load(a.trial/(name+'-raw.npy'));stable=np.load(a.trial/(name+'-stable.npy'))
 off=np.fromfile(a.trial/(name+'-off.rgba32f'),dtype='<f4').reshape(h,w,4)[...,:3];on=np.fromfile(a.trial/(name+'-on.rgba32f'),dtype='<f4').reshape(h,w,4)[...,:3];delta=on-off
 if prior:
  image,oldraw,oldstable,olddelta=prior
  gray=lambda v:np.rint(cv2.cvtColor(v,cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
  flow=cv2.calcOpticalFlowFarneback(gray(rgb),gray(image),None,.5,3,15,3,5,1.2,0)
  xy=np.stack(np.meshgrid(np.arange(w),np.arange(h)),-1).astype(np.float32)+flow
  remap=lambda v:cv2.remap(v,xy[...,0],xy[...,1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
  valid=(xy[...,0]>=0)&(xy[...,0]<=w-1)&(xy[...,1]>=0)&(xy[...,1]<=h-1)&(np.abs(rgb-remap(image)).mean(-1)<.06)
  r,s,d=remap(oldraw),remap(oldstable),remap(olddelta);skin=valid&(np.maximum(raw[...,0],r[...,0])>.1)
  avg=lambda v,m:float(np.mean(v[m])) if m.any() else None
  pairs.append(dict(frame=name,valid_pixels=int(valid.sum()),skin_pixels=int(skin.sum()),raw_mask_mae=avg(np.abs(raw-r).mean(-1),valid),stable_mask_mae=avg(np.abs(stable-s).mean(-1),valid),skin_raw_mae=avg(np.abs(raw[...,0]-r[...,0]),skin),skin_stable_mae=avg(np.abs(stable[...,0]-s[...,0]),skin),added_color_mae=avg(np.abs(delta-d).mean(-1),valid),skin_added_color_mae=avg(np.abs(delta-d).mean(-1),skin)))
 prior=(rgb,raw,stable,delta)
summary={k:float(np.mean([x[k] for x in pairs if x[k] is not None])) for k in ['raw_mask_mae','stable_mask_mae','skin_raw_mae','skin_stable_mae','added_color_mae','skin_added_color_mae']}
summary['worst_skin_pairs']=sorted([x for x in pairs if x['skin_added_color_mae'] is not None],key=lambda x:x['skin_added_color_mae'],reverse=True)[:5]
(a.trial/'motion-review.json').write_text(json.dumps(dict(summary=summary,pairs=pairs,limitation='Motion-compensated diagnostics only; all errors depend on estimated flow. No ground truth or perceptual flicker approval.'),indent=2))
# Labelled comparison from exact native-rendered images. No interpolation.
writer=None
for path in frames:
 name=path.stem;left=Image.open(a.trial/(name+'-off.png'));right=Image.open(a.trial/(name+'-on.png'));w,h=left.size
 panel=Image.new('RGB',(w*2,h+28),(18,18,18));panel.paste(left,(0,28));panel.paste(right,(w,28));d=ImageDraw.Draw(panel);d.text((8,8),'Ordinary LUT',fill='white');d.text((w+8,8),'Scene Smart - prototype',fill='white')
 if writer is None:writer=cv2.VideoWriter(str(a.trial/'Scene-Smart-Comparison.mp4'),cv2.VideoWriter_fourcc(*'mp4v'),a.fps,panel.size)
 if not writer.isOpened():raise RuntimeError('Video writer failed')
 writer.write(cv2.cvtColor(np.asarray(panel),cv2.COLOR_RGB2BGR))
writer.release();cap=cv2.VideoCapture(str(a.trial/'Scene-Smart-Comparison.mp4'));count=0
while cap.read()[0]:count+=1
cap.release()
if count!=len(frames):raise RuntimeError('Encoded video incomplete')
print(json.dumps(summary,indent=2));print('Decoded video frames',count)
