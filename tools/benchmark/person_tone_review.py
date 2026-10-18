"""Bound comparisons to exact native outputs; flow diagnostics are not perceptual QA."""
import argparse,json,hashlib
from pathlib import Path
import cv2,numpy as np
from PIL import Image,ImageDraw,ImageOps
p=argparse.ArgumentParser();p.add_argument('trial',type=Path);p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(exist_ok=False)
e=json.loads((a.trial/'evidence.json').read_text());sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest();prior=None;pairs=[];video=None
for index,record in enumerate(e['cases']):
 name=record['name'];binding=json.loads((a.source/'people-reviewed'/(name+'.json')).read_text())
 if sha(a.source/'people-reviewed'/(name+'.json'))!=record['analysis_sha256'] or sha(a.trial/(name+'-individual.rgba32f'))!=record['result_sha256']:raise ValueError('Changed trial/analysis')
 source=Image.open(a.source/'people-motion'/(name+'.png')).convert('RGB');rgb=np.array(source,np.float32)/255;h,w=rgb.shape[:2]
 base=np.fromfile(a.trial/(name+'-ordinary.rgba32f'),np.float32).reshape(h,w,4)[...,:3];on=np.fromfile(a.trial/(name+'-individual.rgba32f'),np.float32).reshape(h,w,4)[...,:3];delta=on-base
 fields=[]
 for person in record['people']:
  if sha(person['mask'])!=person['mask_sha256']:raise ValueError('Changed mask')
  fields.append(np.fromfile(person['mask'],np.float32).reshape(h,w))
 mask=np.sum(fields,axis=0) if fields else np.zeros((h,w),np.float32)
 if prior is not None:
  old_rgb,old_delta,old_mask=prior
  gray=lambda v:np.rint(cv2.cvtColor(v,cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
  flow=cv2.calcOpticalFlowFarneback(gray(rgb),gray(old_rgb),None,.5,3,15,3,5,1.2,0)
  xy=np.stack(np.meshgrid(np.arange(w),np.arange(h)),-1).astype(np.float32)+flow
  remap=lambda v:cv2.remap(v,xy[...,0],xy[...,1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
  valid=(xy[...,0]>=0)&(xy[...,0]<=w-1)&(xy[...,1]>=0)&(xy[...,1]<=h-1)&(np.abs(rgb-remap(old_rgb)).mean(-1)<.06)&(np.maximum(mask,remap(old_mask))>.1)
  if valid.any():pairs.append(dict(frame=name,pixels=int(valid.sum()),mask_motion_mae=float(np.abs(mask-remap(old_mask))[valid].mean()),added_color_motion_mae=float(np.abs(delta-remap(old_delta))[valid].mean())))
 prior=(rgb,delta,mask)
 enc=lambda v:np.where(v<=.0031308,v*12.92,1.055*np.maximum(v,0)**(1/2.4)-.055)
 images=[source,*[Image.fromarray(np.rint(np.clip(enc(v),0,1)*255).astype(np.uint8)) for v in (base,on)],Image.fromarray(np.rint(np.clip(mask,0,1)*255).astype(np.uint8)).convert('RGB')]
 board=Image.new('RGB',(960,780),'#121212');draw=ImageDraw.Draw(board);draw.text((8,8),f'Automatic face crops / frame {index} / research, 12 fps',fill='white')
 for row,face in enumerate(sorted(binding['raw_face_details']['faces'],key=lambda f:f['box'][0])[:3]):
  x0,y0,x1,y1=face['box'];pad=.15*(y1-y0);box=(int(x0-pad),int(y0-pad),int(x1+pad),int(y1+pad))
  for col,(image,label) in enumerate(zip(images,['Source','Ordinary LUT','Individual skin','Soft ownership'])):
   crop=ImageOps.contain(image.crop(box),(230,210));board.paste(crop,(col*240+(230-crop.width)//2,30+row*240+(210-crop.height)//2));draw.text((col*240+6,246+row*240),label,fill='white')
 draw.text((8,760),'Tears of Steel / Blender Foundation & Institute / CC BY 3.0 / diagnostic crops',fill='white');board.save(a.output/f'face-{index:03}.png')
 if video is None:video=cv2.VideoWriter(str(a.output/'Individual-Skin-Comparison.mp4'),cv2.VideoWriter_fourcc(*'mp4v'),12,board.size)
 if not video.isOpened():raise RuntimeError('Video writer')
 video.write(cv2.cvtColor(np.array(board),cv2.COLOR_RGB2BGR))
video.release();cap=cv2.VideoCapture(str(a.output/'Individual-Skin-Comparison.mp4'));count=0
while cap.read()[0]:count+=1
cap.release()
if count!=len(e['cases']):raise ValueError('Video frame count')
(a.output/'motion.json').write_text(json.dumps(dict(frames=count,pairs=pairs,mean_added_color_motion_mae=float(np.mean([v['added_color_motion_mae'] for v in pairs])),mean_mask_motion_mae=float(np.mean([v['mask_motion_mae'] for v in pairs])),worst_pairs=sorted(pairs,key=lambda r:r['added_color_motion_mae'],reverse=True)[:5],limitation='Estimated flow; diagnostic only, not continuous perceptual approval or ground truth',trial_sha256=sha(a.trial/'evidence.json')),indent=2))
print('Decoded frames',count)
