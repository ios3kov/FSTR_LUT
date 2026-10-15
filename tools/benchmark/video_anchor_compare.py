"""Same-input native comparison of baseline, flow and SAM2 proposals."""
import argparse,json,hashlib,subprocess
from pathlib import Path
import cv2
import numpy as np
from PIL import Image,ImageDraw
from video_anchor_analysis import combine_prediction,cut_between,combine_geometry
from scene_trial import decode,encode
p=argparse.ArgumentParser()
for key in ('trial','baseline','renderer','lut'):p.add_argument(key,type=Path)
p.add_argument('--output',default='comparison-final');p.add_argument('--flow',default='flow-final');p.add_argument('--sam2',default='sam2-final')
a=p.parse_args();out=a.trial/a.output;out.mkdir(exist_ok=False);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();manifest=json.loads((a.trial/'manifest.json').read_text());images=[];raw=[];records=[]
for folder in (a.flow,a.sam2):
 evidence=json.loads((a.trial/folder/'evidence.json').read_text())
 if evidence['manifest_sha256']!=sha(a.trial/'manifest.json'):raise ValueError('Prediction belongs to different inputs')
 for anchor in manifest['anchors']:
  direction=anchor['direction'];start=anchor['index']
  for index in range(len(manifest['cases'])):
   if (direction=='forward' and index<start) or (direction=='backward' and index>start):continue
   names=[f'{direction}-{index:03}.npy']
   if folder==a.flow:names.append(f'{direction}-{index:03}-trust.npy')
   if any(name not in evidence['output_sha256'] for name in names):raise ValueError('Incomplete prediction provenance')
 for name,digest in evidence['output_sha256'].items():
  if sha(a.trial/folder/name)!=digest:raise ValueError('Changed prediction cache')
for c in manifest['cases']:
 if sha(Path(c['path']))!=c['sha256'] or sha(Path(c['raw_path']))!=c['raw_sha256'] or sha(Path(c['people_path']))!=c['people_sha256']:raise ValueError('Changed evidence')
 images.append(np.asarray(Image.open(c['path']),dtype=np.float32)/255);raw.append(np.load(c['raw_path'],allow_pickle=False))
lo,hi=[x['index'] for x in manifest['anchors']];amplitude=float(np.mean([x['amplitude'] for x in manifest['anchors']]))
for i,(c,rgb) in enumerate(zip(manifest['cases'],images)):
 h,w=rgb.shape[:2];base=np.load(a.baseline/(c['name']+'-stable.npy'),allow_pickle=False)
 with np.load(c['people_path'],allow_pickle=False) as v:pose=v['raw_pose_masks'];person=pose.max(0) if len(pose) else np.zeros((h,w),np.float32)
 person=np.maximum(person,raw[i][...,:3].sum(-1)).clip(0,1)
 fields={'baseline':base};detail={};supported=lo<=i<=hi and not cut_between(images,lo,hi)
 for mode,folder in [('flow',a.flow),('sam2',a.sam2),('hybrid',a.sam2)]:
  if supported:
   forward=np.load(a.trial/folder/f'forward-{i:03}.npy',allow_pickle=False);backward=np.load(a.trial/folder/f'backward-{i:03}.npy',allow_pickle=False)
   trust=np.ones((h,w),np.float32)
   if mode=='flow':trust=np.minimum(np.load(a.trial/folder/f'forward-{i:03}-trust.npy'),np.load(a.trial/folder/f'backward-{i:03}-trust.npy'))
   if mode=='hybrid':
    ff=np.load(a.trial/a.flow/f'forward-{i:03}.npy',allow_pickle=False);fb=np.load(a.trial/a.flow/f'backward-{i:03}.npy',allow_pickle=False)
    ft=np.minimum(np.load(a.trial/a.flow/f'forward-{i:03}-trust.npy'),np.load(a.trial/a.flow/f'backward-{i:03}-trust.npy'))
    forward,backward,contribution=combine_geometry(forward,backward,ff,fb,ft)
    np.save(out/f'{c["name"]}-flow-contribution.npy',contribution)
   fields[mode],weight=combine_prediction(base,forward,backward,trust,person,amplitude)
   agreed=np.minimum(forward,backward);negative=(base[...,2]>.7)&(raw[i][...,0]<.05)
   detail[mode]=dict(supported_pixels=int((weight>.05).sum()),changed_pixels=int((np.abs(fields[mode]-base).max(-1)>1e-6).sum()),direction_iou=float(np.logical_and(forward>.5,backward>.5).sum()/max(1,np.logical_or(forward>.5,backward>.5).sum())),proposal_on_clothing=float(agreed[negative].mean()) if negative.any() else 0,final_added_skin_on_clothing=float(np.maximum(fields[mode][...,0]-base[...,0],0)[negative].mean()) if negative.any() else 0)
   np.save(out/f'{c["name"]}-{mode}-weight.npy',weight)
  else:fields[mode]=base.copy();detail[mode]=dict(changed_pixels=0,fallback=True)
 source=np.concatenate([decode(rgb),np.ones((h,w,1),np.float32)],-1).astype('<f4');source.tofile(out/'input.rgba32f');row=Image.new('RGB',(w*4,h*2+28),(18,18,18));draw=ImageDraw.Draw(row)
 for k,(mode,field) in enumerate(fields.items()):
  np.save(out/f'{c["name"]}-{mode}-mask.npy',field);field.astype('<f4').tofile(out/'weights.f32');target=out/f'{c["name"]}-{mode}.rgba32f'
  subprocess.run([str(a.renderer.resolve()),str(a.lut.resolve()),str(w),str(h),str(out/'input.rgba32f'),str(out/'weights.f32'),str(target)],check=True)
  rgba=np.fromfile(target,dtype='<f4').reshape(h,w,4)
  if not np.isfinite(rgba).all() or not np.all(rgba[...,3]==1):raise RuntimeError('Invalid native render')
  im=Image.fromarray(np.rint(np.clip(encode(rgba[...,:3]),0,1)*255).astype(np.uint8));im.save(out/f'{c["name"]}-{mode}.png');row.paste(im,(k*w,28));row.paste(Image.fromarray(np.rint(field[...,0]*255).astype(np.uint8)).convert('RGB'),(k*w,h+28));draw.text((k*w+5,7),mode+' / '+str(c['frame']),fill='white')
 if sha(out/f'{c["name"]}-baseline.rgba32f')!=sha(a.baseline/(c['name']+'-on.rgba32f')):raise RuntimeError('Baseline mismatch')
 row.save(out/(c['name']+'-review.jpg'),quality=95);records.append(dict(frame=c['frame'],baseline_mask_sha256=sha(a.baseline/(c['name']+'-stable.npy')),baseline_reference_sha256=sha(a.baseline/(c['name']+'-on.rgba32f')),baseline_exact=True,supported=supported,modes=detail));print(c['name'],detail,flush=True)
(out/'evidence.json').write_text(json.dumps(dict(manifest_sha256=sha(a.trial/'manifest.json'),renderer_sha256=sha(a.renderer),lut_sha256=sha(a.lut),prediction_evidence_sha256={folder:sha(a.trial/folder/'evidence.json') for folder in (a.flow,a.sam2)},output_sha256={x.name:sha(x) for x in out.iterdir() if x.is_file()},amplitude=amplitude,cases=records,limitations='Clothing metric uses existing semantic labels as a diagnostic proxy, not truth. Same auto-selected component at two anchors is a single-person trial, not general identity matching.'),indent=2))
