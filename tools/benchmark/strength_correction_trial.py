"""Research integration: per-person scalar anchors, current-frame color and geometry.

No native/AE installation; bounded interpolation only inside confirmed face support.
"""
import argparse, hashlib, json, shutil, subprocess
from pathlib import Path
import cv2
import numpy as np
from PIL import Image
from reference_correction import stable_strengths

p=argparse.ArgumentParser()
for k in ('baseline','source','support','output'):p.add_argument('--'+k,type=Path,required=True)
p.add_argument('--raw-strength',action='store_true')
p.add_argument('--renderer',type=Path,required=True)
p.add_argument('--lut',type=Path,required=True)
a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
producer_files=[Path(__file__),Path(__file__).with_name('reference_correction.py'),a.renderer,a.lut]
producer_bindings={str(p.resolve()):sha(p) for p in producer_files}
(a.output/'producer.py').write_bytes(Path(__file__).read_bytes())
e=json.loads((a.baseline/'evidence.json').read_text());records=e['cases'];frames=[];tracks={}
if sha(a.lut)!=e['lut_sha256']:raise ValueError('Different LUT from baseline')
for i,r in enumerate(records):
 name=r['name'];path=a.baseline/(name+'-individual.rgba32f')
 if sha(path)!=r['result_sha256']:raise ValueError('Changed baseline')
 binding_path=a.source/'people-reviewed'/(name+'.json')
 if sha(binding_path)!=r['analysis_sha256']:raise ValueError('Changed people analysis')
 binding=json.loads(binding_path.read_text())
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
  if sha(person['mask'])!=person['mask_sha256'] or sha(person['samples'])!=person['samples_sha256']:raise ValueError('Changed mask/reference')
  mask=np.fromfile(person['mask'],np.float32).reshape(h,w)[y0:y1,x0:x1];geometry=g[y0:y1,x0:x1]
  selected=geometry>.25
  if selected.sum()<8:continue
  score=float(np.clip(np.mean(mask[selected]/np.maximum(geometry[selected],1e-6)),0,1))
  tracks.setdefault(t,{})[i]=dict(box=(x0,y0,x1,y1),score=score,confidence=float(face['detector_score']),samples=person['samples'])
 frames.append((on,base,g))
schedules={};anchors={}
for t,items in tracks.items():
 schedules[t],anchors[t]=stable_strengths({i:v['score'] for i,v in items.items()},{i:v['confidence'] for i,v in items.items()},e.get('cut_before',[]))
if a.raw_strength:
 for t,items in tracks.items():schedules[t]={i:v['score'] for i,v in items.items()}
updates=[]
for i,(r,(on,base,g)) in enumerate(zip(records,frames)):
 name=r['name'];h,w=g.shape;delta=(on-base)[...,:3];acc=np.zeros_like(delta);mass=np.zeros(g.shape,np.float32);used=[]
 # Current material veto is retained; no propagation of old pixels or old contours.
 binding=json.loads((a.source/'people-reviewed'/(name+'.json')).read_text());parts_path=a.source/'parts'/(name+'.npy')
 if sha(parts_path)!=binding['parts']['parts_sha256']:raise ValueError('Changed material evidence')
 parts=np.load(parts_path,allow_pickle=False);non_skin=np.clip((parts[...,0]+parts[...,1]+parts[...,4]-.6)/.3,0,1);veto=1-non_skin*non_skin*(3-2*non_skin)
 for t,items in tracks.items():
  if i not in anchors[t]:continue
  v=items[i];x0,y0,x1,y1=v['box'];geometry=np.zeros((h,w),np.float32);geometry[y0:y1,x0:x1]=g[y0:y1,x0:x1]*veto[y0:y1,x0:x1]
  # The native core evaluates the current input through this person's fixed
  # LUT-directed reference at full weight. Only a scalar is averaged in time.
  mask_path=a.output/(name+f'-full-{t}.mask');np.ones((h,w),np.float32).tofile(mask_path)
  target=a.output/(name+f'-full-{t}.rgba32f');job=dict(width=w,height=h,input=str((a.baseline/(name+'.rgba32f')).resolve()),output=str(target.resolve()),lut=str(a.lut.resolve()),strength=1.,enabled=True,people=[dict(mask=str(mask_path.resolve()),samples=v['samples'])])
  job_path=a.output/(name+f'-full-{t}.json');job_path.write_text(json.dumps(job));subprocess.run([str(a.renderer.resolve()),str(job_path.resolve())],check=True,capture_output=True,timeout=60)
  full=np.fromfile(target,np.float32).reshape(on.shape)[...,:3]-base[...,:3]
  acc+=full*(geometry*schedules[t][i])[...,None];mass+=geometry
  used.append(dict(track=t,strength=schedules[t][i],raw_strength=v['score'],anchor=anchors[t][i]))
 # Convex ownership in overlaps; retain all upstream corrections outside faces.
 delta=(1-np.minimum(1,mass))[...,None]*delta+acc/np.maximum(1,mass)[...,None]
 delta=delta.astype(np.float64);delta-=(delta@np.array([.2126,.7152,.0722]))[...,None]
 b=base[...,:3].astype(np.float64);scale=np.ones(g.shape)
 for c in range(3):
  d=delta[...,c];positive=d>0;negative=d<0
  scale[positive]=np.minimum(scale[positive],(1-b[...,c][positive])/d[positive]);scale[negative]=np.minimum(scale[negative],-b[...,c][negative]/d[negative])
 valid=((b>=0)&(b<=1)).all(-1);scale=np.where(valid,np.clip(scale,0,1),0)
 result=on.copy();select=mass>0;replacement=(b+delta*scale[...,None]).astype(np.float32);result[select,:3]=replacement[select]
 if not np.isfinite(result).all() or not np.array_equal(result[...,3],on[...,3]):raise ValueError('Output contract')
 out=a.output/(name+'-individual.rgba32f');result.tofile(out)
 for mode in ('ordinary','off'):shutil.copyfile(a.baseline/(name+'-'+mode+'.rgba32f'),a.output/(name+'-'+mode+'.rgba32f'))
 pixels=np.fromfile(a.baseline/(name+'.rgba32f'),np.float32).reshape(on.shape)
 enc=lambda v:np.where(v<=.0031308,v*12.92,1.055*np.maximum(v,0)**(1/2.4)-.055)
 image=a.output/(name+'-comparison.png');Image.fromarray(np.rint(np.clip(np.concatenate([enc(v[...,:3]) for v in (pixels,base,result)],1),0,1)*255).astype(np.uint8)).save(image)
 updates.append(dict(r,result_sha256=sha(out),comparison_sha256=sha(image),anchors_used=used,max_luminance_error=float(np.max(np.abs((result[...,:3]-base[...,:3])@np.array([.2126,.7152,.0722]))))))
 print(name,'people',len(used),flush=True)
if any(sha(path)!=value for path,value in producer_bindings.items()):raise ValueError('Producer changed during trial')
e.update(producer_bindings=producer_bindings,raw_strength=a.raw_strength,cases=updates,strength_anchors=anchors,reference_source_sha256=sha(a.baseline/'evidence.json'),reference_runner_sha256=sha(__file__),reference_algorithm_sha256=sha(Path(__file__).with_name('reference_correction.py')),renderer_sha256=sha(a.renderer),lut_sha256=sha(a.lut),scope='Research per-person scalar strength interpolation; current-frame native color and geometry; no AE acceptance')
for r in updates:r.pop('reverse_exact',None)
(a.output/'evidence.json').write_text(json.dumps(e,indent=2))
