"""Compare both methods on identical flow/appearance-valid pixel sets."""
import argparse,json,hashlib
from pathlib import Path
import cv2,numpy as np
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('baseline',type=Path);p.add_argument('candidate',type=Path);p.add_argument('support',type=Path);p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
records=[json.loads((d/'evidence.json').read_text())['cases'] for d in (a.baseline,a.candidate)]
if [c['name'] for c in records[0]]!=[c['name'] for c in records[1]]:raise ValueError('Different frame sequence')
prior=None;rows=[]
for pair in zip(*records):
 name=pair[0]['name'];image=a.source/'people-motion'/(name+'.png')
 if any(sha(image)!=c['source_sha256'] for c in pair):raise ValueError('Changed source')
 rgb=np.array(Image.open(image).convert('RGB'),np.float32)/255;h,w=rgb.shape[:2];values=[];plain=None
 for d,c in zip((a.baseline,a.candidate),pair):
  path=d/(name+'-individual.rgba32f')
  if sha(path)!=c['result_sha256']:raise ValueError('Changed output')
  on=np.fromfile(path,np.float32).reshape(h,w,4)[...,:3];off=np.fromfile(d/(name+'-ordinary.rgba32f'),np.float32).reshape(h,w,4)[...,:3]
  if plain is not None and not np.array_equal(plain,off):raise ValueError('Different ordinary baseline')
  plain=off;mask=np.zeros((h,w),np.float32)
  for person in c['people']:
   if sha(person['mask'])!=person['mask_sha256']:raise ValueError('Changed mask')
   mask+=np.fromfile(person['mask'],np.float32).reshape(h,w)
  values.append((on-off,mask))
 binding=json.loads((a.support/(name+'.json')).read_text());support_path=a.support/(name+'.npz')
 if sha(support_path)!=binding['evidence_sha256'] or binding['source_sha256']!=pair[0]['source_sha256']:raise ValueError('Changed support')
 with np.load(support_path,allow_pickle=False) as f:geometry=f['raw_face_support']
 if prior:
  previous,old_values,old_geometry=prior;gray=lambda v:np.rint(cv2.cvtColor(v,cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
  flow=cv2.calcOpticalFlowFarneback(gray(rgb),gray(previous),None,.5,3,15,3,5,1.2,0);xy=np.stack(np.meshgrid(np.arange(w),np.arange(h)),-1).astype(np.float32)+flow
  remap=lambda v:cv2.remap(v,xy[...,0],xy[...,1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
  valid=(xy[...,0]>=0)&(xy[...,0]<=w-1)&(xy[...,1]>=0)&(xy[...,1]<=h-1)&(np.abs(rgb-remap(previous)).mean(-1)<.06)
  domains={'face_geometry':valid&(np.maximum(geometry,remap(old_geometry))>.1),'mask_union':valid&(np.maximum.reduce([values[0][1],values[1][1],remap(old_values[0][1]),remap(old_values[1][1])])>.1)}
  row=dict(frame=name,domains={})
  for domain,select in domains.items():
   if not select.any():continue
   results=[]
   for (delta,mask),(old_delta,old_mask) in zip(values,old_values):results.append(dict(mask_mae=float(np.abs(mask-remap(old_mask))[select].mean()),color_mae=float(np.abs(delta-remap(old_delta))[select].mean())))
   row['domains'][domain]=dict(pixels=int(select.sum()),baseline=results[0],candidate=results[1])
  rows.append(row)
 prior=(rgb,values,geometry)
summary={}
for domain in ('face_geometry','mask_union'):
 summary[domain]={}
 for metric in ('mask_mae','color_mae'):
  old=float(np.mean([r['domains'][domain]['baseline'][metric] for r in rows if domain in r['domains']]))
  new=float(np.mean([r['domains'][domain]['candidate'][metric] for r in rows if domain in r['domains']]))
  summary[domain][metric]=dict(baseline=old,candidate=new,relative_change=(new/old-1) if old else None)
a.output.write_text(json.dumps(dict(summary=summary,pairs=rows,limitation='Identical domains and estimated flow; no pixel truth, no continuous perceptual acceptance'),indent=2));print(json.dumps(summary,indent=2))
