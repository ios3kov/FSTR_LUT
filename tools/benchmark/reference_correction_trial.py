"""Research integration: automatic per-track anchors, explicit frozen color frames.

No native/AE installation; bounded interpolation only inside confirmed face support.
"""
import argparse, hashlib, json, shutil
from pathlib import Path
import cv2
import numpy as np
from PIL import Image
from reference_correction import anchor_pair, interpolate_correction

p=argparse.ArgumentParser()
for k in ('baseline','source','support','output'):p.add_argument('--'+k,type=Path,required=True)
a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
e=json.loads((a.baseline/'evidence.json').read_text());records=e['cases'];frames=[];tracks={}
for i,r in enumerate(records):
 name=r['name'];path=a.baseline/(name+'-individual.rgba32f')
 if sha(path)!=r['result_sha256']:raise ValueError('Changed baseline')
 binding=json.loads((a.source/'people-reviewed'/(name+'.json')).read_text())
 sb=json.loads((a.support/(name+'.json')).read_text());sp=a.support/(name+'.npz')
 if sb['source_sha256']!=r['source_sha256'] or sha(sp)!=sb['evidence_sha256']:raise ValueError('Changed support')
 with np.load(sp,allow_pickle=False) as f:g=f['raw_face_support'].copy()
 h,w=g.shape;on=np.fromfile(path,np.float32).reshape(h,w,4);base=np.fromfile(a.baseline/(name+'-ordinary.rgba32f'),np.float32).reshape(h,w,4)
 observations=sorted(binding['raw_face_details']['faces'],key=lambda f:tuple(f['box']))
 # Track list is observation-aligned only when all detected observations survived.
 if len(observations)!=len(r['tracks']):raise ValueError('Ambiguous observation mapping')
 persons={v['track']:v for v in r['people']}
 for face,t in zip(observations,r['tracks']):
  if t is None or t not in persons:continue
  x0,y0,x1,y1=face['box'];x0=max(0,int(np.floor(x0)));y0=max(0,int(np.floor(y0)));x1=min(w,int(np.ceil(x1)));y1=min(h,int(np.ceil(y1)))
  if x1<=x0 or y1<=y0:continue
  person=persons[t]
  if sha(person['mask'])!=person['mask_sha256']:raise ValueError('Changed mask')
  mask=np.fromfile(person['mask'],np.float32).reshape(h,w)[y0:y1,x0:x1];geometry=g[y0:y1,x0:x1]
  selected=geometry>.25
  if selected.sum()<8:continue
  score=float(mask[selected].mean())*float(face['detector_score'])
  tracks.setdefault(t,{})[i]=dict(box=(x0,y0,x1,y1),score=score,delta=cv2.resize((on-base)[y0:y1,x0:x1,:3],(64,64)),geometry=cv2.resize(geometry,(64,64)))
 frames.append((on,base,g))
anchors={}
for t,items in tracks.items():
 anchors[t]=[i for i,v in items.items() if v['score']>=.25 and all(v['score']>=items[j]['score'] for j in (i-1,i+1) if j in items)]
updates=[]
for i,(r,(on,base,g)) in enumerate(zip(records,frames)):
 delta=(on-base)[...,:3];acc=np.zeros_like(delta);mass=np.zeros(g.shape,np.float32);used=[]
 for t,items in tracks.items():
  pair=anchor_pair(i,anchors[t],set(items),e.get('cut_before',[]))
  if pair is None:continue
  l,rgt,f=pair;v=items[i];x0,y0,x1,y1=v['box'];size=(x1-x0,y1-y0)
  left=cv2.resize(items[l]['delta'],size);right=cv2.resize(items[rgt]['delta'],size)
  support=np.minimum.reduce([g[y0:y1,x0:x1],cv2.resize(items[l]['geometry'],size),cv2.resize(items[rgt]['geometry'],size)])
  support=np.clip(support,0,1)
  acc[y0:y1,x0:x1]+=((1-f)*left+f*right)*support[...,None];mass[y0:y1,x0:x1]+=support
  used.append(dict(track=t,left=l,right=rgt,fraction=f))
 target=acc/np.maximum(mass[...,None],1e-12)
 delta=interpolate_correction(delta,target,target,.5,np.minimum(1,mass))
 # Remove numerical luminance drift; keep original LUT luminance and alpha.
 delta=delta.astype(np.float64);delta-=(delta@np.array([.2126,.7152,.0722]))[...,None]
 b=base[...,:3].astype(np.float64);scale=np.ones(g.shape)
 for c in range(3):
  d=delta[...,c];positive=d>0;negative=d<0
  scale[positive]=np.minimum(scale[positive],(1-b[...,c][positive])/d[positive])
  scale[negative]=np.minimum(scale[negative],-b[...,c][negative]/d[negative])
 valid=((b>=0)&(b<=1)).all(-1);scale=np.where(valid,np.clip(scale,0,1),0)
 result=on.copy();replacement=(b+delta*scale[...,None]).astype(np.float32)
 select=mass>0;result[select,:3]=replacement[select]
 if not np.isfinite(result).all() or not np.array_equal(result[...,3],on[...,3]):raise ValueError('Output contract')
 name=r['name'];out=a.output/(name+'-individual.rgba32f');result.tofile(out)
 for mode in ('ordinary','off'):shutil.copyfile(a.baseline/(name+'-'+mode+'.rgba32f'),a.output/(name+'-'+mode+'.rgba32f'))
 inp=a.baseline/(name+'.rgba32f');pixels=np.fromfile(inp,np.float32).reshape(on.shape)
 enc=lambda v:np.where(v<=.0031308,v*12.92,1.055*np.maximum(v,0)**(1/2.4)-.055)
 image=a.output/(name+'-comparison.png');Image.fromarray(np.rint(np.clip(np.concatenate([enc(v[...,:3]) for v in (pixels,base,result)],1),0,1)*255).astype(np.uint8)).save(image)
 updates.append(dict(r,result_sha256=sha(out),comparison_sha256=sha(image),anchors_used=used,max_luminance_error=float(np.max(np.abs((result[...,:3]-base[...,:3])@np.array([.2126,.7152,.0722]))))))
e.update(cases=updates,reference_anchors=anchors,reference_source_sha256=sha(a.baseline/'evidence.json'),reference_runner_sha256=sha(__file__),reference_algorithm_sha256=sha(Path(__file__).with_name('reference_correction.py')),scope='Research anchor interpolation of added correction; no AE acceptance; inherited reverse_exact describes upstream renderer only')
for r in updates:r.pop('reverse_exact',None)
(a.output/'evidence.json').write_text(json.dumps(e,indent=2));print(json.dumps(dict(anchors=anchors,frames_changed=sum(bool(r['anchors_used']) for r in updates))))
