"""Check native preprocessing + UperNet + probability fusion against PyTorch."""
import argparse,json,subprocess,hashlib
from pathlib import Path
import numpy as np
import torch
from transformers import UperNetForSemanticSegmentation
from scene_analysis import GROUPS,information_support,refine_weights,smooth
p=argparse.ArgumentParser()
for k in ['probe','model','onnx','parts','output']:p.add_argument('--'+k,type=Path,required=True)
a=p.parse_args();a.output.mkdir(exist_ok=False);torch.set_num_threads(4)
model=UperNetForSemanticSegmentation.from_pretrained(a.model,local_files_only=True).eval();labels={int(k):v.strip() for k,v in model.config.id2label.items()};records=[]
for raw in sorted(a.parts.glob('*.rgba8')):
 index=raw.stem;nativeparts=a.parts/(index+'.parts');record=json.loads((a.parts/'verification.json').read_text())['cases'][int(index)];h,w,_=record['shape'];rgb=np.fromfile(raw,dtype='uint8').reshape(h,w,4)[...,:3];parts=np.fromfile(nativeparts,dtype='<f4').reshape(h,w,6)
 # Independent vectorized implementation of the declared native half-pixel
 # bilinear + uint8 rounding, intentionally separate from PIL antialias resize.
 yy=np.clip((np.arange(512)+.5)*h/512-.5,0,h-1);xx=np.clip((np.arange(512)+.5)*w/512-.5,0,w-1);y0=yy.astype(int);x0=xx.astype(int);y1=np.minimum(y0+1,h-1);x1=np.minimum(x0+1,w-1);fy=(yy-y0)[:,None,None];fx=(xx-x0)[None,:,None]
 resized=np.floor((1-fy)*((1-fx)*rgb[y0[:,None],x0]+fx*rgb[y0[:,None],x1])+fy*((1-fx)*rgb[y1[:,None],x0]+fx*rgb[y1[:,None],x1])+.5)
 normalized=((resized/255-np.array([.485,.456,.406]))/np.array([.229,.224,.225])).astype('float32').transpose(2,0,1)[None]
 with torch.inference_mode():logits=model(pixel_values=torch.from_numpy(normalized)).logits;probs=torch.nn.functional.interpolate(logits,size=(h,w),mode='bilinear',align_corners=False).softmax(1)[0].numpy()
 roles=np.zeros((h,w,8),np.float32)
 for role,names in GROUPS.items():roles[...,role]=probs[[i for i,name in labels.items() if name in names]].sum(0)
 roles[...,0]=probs[12]*(parts[...,2]+parts[...,3]);roles[...,1]=probs[12]*parts[...,1];roles[...,2]=probs[12]*parts[...,4];roles*=smooth(.35,.8,probs.max(0))[...,None]*information_support(rgb/255.);roles=refine_weights((rgb/255.).astype('float32'),roles)
 out=a.output/(index+'.roles');person=a.output/(index+'.person');run=subprocess.run([str(a.probe.resolve()),str(a.onnx.resolve()),str(w),str(h),str(raw.resolve()),str(nativeparts.resolve()),str(out.resolve()),str(person.resolve())],capture_output=True,text=True);(a.output/(index+'.stderr')).write_text(run.stderr)
 if run.returncode:raise RuntimeError(run.stderr)
 actual=np.fromfile(out,dtype='<f4').reshape(h,w,8);ap=np.fromfile(person,dtype='<f4').reshape(h,w);err=float(np.abs(actual-roles).max());pe=float(np.abs(ap-probs[12]).max());roles.tofile(a.output/(index+'.reference.roles'));r={'source':record['source'],'roles_max_abs':err,'person_max_abs':pe,'tolerance':5e-5};records.append(r);print(r,flush=True)
 if max(err,pe)>5e-5:raise ValueError('Native scene differs')
(a.output/'verification.json').write_text(json.dumps({'cases':records,'preprocessing':'Native half-pixel bilinear uint8; differs from original PIL antialias kernel; requires new visual acceptance','probe_sha256':hashlib.sha256(a.probe.read_bytes()).hexdigest(),'manifest_sha256':hashlib.sha256((a.onnx/'manifest.json').read_bytes()).hexdigest()},indent=2))
