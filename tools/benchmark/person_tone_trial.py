"""Automatic face-confirmed individual-tone research trial, no manual targets.

Uses existing detector/skin evidence. Does not claim exposed hands or body identity.
"""
import argparse,hashlib,json,subprocess
from pathlib import Path
import numpy as np
from PIL import Image
from person_tone_analysis import observation,associate,ownership,attach_body_skin,stabilize_fields
from scene_analysis import stabilize_window
from scene_people import face_field
from video_anchor_analysis import cut_between
p=argparse.ArgumentParser()
for key in ('source','output','renderer','lut'):p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--cut-before',type=int,action='append',default=[])
a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest()
e=json.loads((a.source/'combined/evidence.json').read_text());expected={r['name']:r for r in e['cases']};cases=e['sources']['cases']
if not 1<=len(cases)<=256:raise ValueError('Explicit frame-window budget')
frames=[];rgba=[];sources=[];bindings=[];raw_masks=[];rgb_images=[];scene_raw=[]
for case in cases:
 name=case['name']
 if Path(name).name!=name:raise ValueError('Invalid frame name')
 path=Path(case['path']);npz=a.source/'people-reviewed'/(name+'.npz');binding=json.loads((a.source/'people-reviewed'/(name+'.json')).read_text())
 raw_path=a.source/'combined'/(name+'-raw.npy')
 if sha(raw_path)!=expected[name]['raw_sha256']:raise ValueError('Changed scene evidence')
 scene_raw.append(np.load(raw_path,allow_pickle=False))
 if binding!=expected[name]['people'] or binding['source_sha256']!=case['sha256'] or sha(path)!=case['sha256'] or sha(npz)!=binding['evidence_sha256']:raise ValueError('Changed source/analysis evidence')
 rgb=np.array(Image.open(path).convert('RGB'),dtype=np.float32)/255
 linear=np.where(rgb<=.04045,rgb/12.92,((rgb+.055)/1.055)**2.4).astype(np.float32)
 pixels=np.concatenate([linear,np.ones((*rgb.shape[:2],1),np.float32)],axis=2)
 with np.load(npz,allow_pickle=False) as f:mask=f['raw_face_mask'].copy();poses=f['raw_pose_masks'].copy()
 parts_path=a.source/'parts'/(name+'.npy')
 if sha(parts_path)!=binding['parts']['parts_sha256'] or binding['parts']['source_sha256']!=case['sha256']:raise ValueError('Changed body-material evidence')
 parts=np.load(parts_path,allow_pickle=False)
 observations=[]
 for face in sorted(binding['raw_face_details']['faces'],key=lambda r:tuple(r['box'])):
  field,detail=face_field(rgb,mask,[face],case['source_height']/rgb.shape[0])
  if not detail:continue
  o=observation(face['box'],field,linear,float(face['detector_score']))
  if o is not None:observations.append(o)
 observations=attach_body_skin(observations,poses,parts,linear)
 frames.append(observations);rgba.append(pixels);rgb_images.append(rgb);sources.append(path);bindings.append(binding);raw_masks.append(mask)
cuts=sorted(set(a.cut_before)|{i for i in range(1,len(frames)) if cut_between({0:rgb_images[i-1],1:rgb_images[i]},0,1)})
ids=associate(frames,cut_before=cuts)
collected={}
for index,(obs,tracks) in enumerate(zip(frames,ids)):
 for o,track in zip(obs,tracks):
  if track is None:continue
  valid=(o['mask']>.25)&(raw_masks[index]>.7)
  x0,y0=np.floor(o['box'][:2]).astype(int);x1,y1=np.ceil(o['box'][2:]).astype(int);inside=np.zeros(valid.shape,bool);inside[max(0,y0):min(valid.shape[0],y1),max(0,x0):min(valid.shape[1],x1)]=True;valid&=inside
  color=rgba[index][valid,:3];confidence=np.minimum(o['mask'][valid],raw_masks[index][valid])*o['confidence']
  # Deterministic spatial subsampling, never a frame-dependent random sample.
  step=max(1,int(np.ceil(len(color)/512)))
  samples=np.column_stack([color[::step],confidence[::step]]).astype(np.float32)
  if len(samples):collected.setdefault(track,[]).append(samples)
references={}
for track,parts in collected.items():
 samples=np.concatenate(parts);step=max(1,int(np.ceil(len(samples)/32768)));target=a.output/f'person-{track}.samples';samples[::step].tofile(target);references[track]=target
render_frames=stabilize_fields(rgb_images,frames,ids,cuts)
records=[]
for index,(obs,tracks,pixels,path) in enumerate(zip(render_frames,ids,rgba,sources)):
 h,w=pixels.shape[:2];stem=cases[index]['name'];input_path=a.output/(stem+'.rgba32f');pixels.tofile(input_path)
 active=[(o,t) for o,t in zip(obs,tracks) if t is not None and t in references]
 fields=ownership(np.stack([o['mask'] for o,t in active])) if active else np.empty((0,h,w),np.float32)
 scene_window={}
 for offset in (-2,-1,0,1,2):
  j=index+offset
  if 0<=j<len(cases) and not any(k in cuts for k in range(min(j,index)+1,max(j,index)+1)):scene_window[offset]=(rgb_images[j],scene_raw[j])
 scene=stabilize_window(scene_window);scene_path=a.output/(stem+'-scene.f32');scene.astype(np.float32).tofile(scene_path)
 if len(fields):fields*=np.minimum(1.,scene[...,0]/np.maximum(fields.sum(0),1e-12))[None]
 people=[]
 for field,(o,track) in zip(fields,active):
  target=a.output/(stem+f'-person-{track}.mask');field.tofile(target);people.append(dict(track=track,mask=str(target.resolve()),mask_sha256=sha(target),samples=str(references[track].resolve()),samples_sha256=sha(references[track])))
 job=dict(width=w,height=h,input=str(input_path.resolve()),lut=str(a.lut.resolve()),strength=1.,people=people,scene=str(scene_path.resolve()))
 outputs={}
 for mode in ('ordinary','individual','off'):
  target=a.output/(stem+'-'+mode+'.rgba32f');job_path=a.output/(stem+'-'+mode+'.json')
  run_job={**job,'output':str(target.resolve()),'people':[] if mode=='ordinary' else people,'enabled':mode=='individual'}
  job_path.write_text(json.dumps(run_job));subprocess.run([str(a.renderer.resolve()),str(job_path.resolve())],check=True,capture_output=True,timeout=60)
  outputs[mode]=np.fromfile(target,np.float32).reshape(h,w,4)
 if not np.array_equal(outputs['ordinary'],outputs['off']):raise ValueError('Smart Off differs')
 result=outputs['individual'];base=outputs['ordinary'];dy=np.max(np.abs((result[...,:3]-base[...,:3])@np.array([.2126,.7152,.0722])))
 if not np.isfinite(result).all() or not np.array_equal(result[...,3],pixels[...,3]) or dy>1e-7:raise ValueError('Color contract failure')
 def encoded(v):return np.where(v<=.0031308,v*12.92,1.055*np.maximum(v,0)**(1/2.4)-.055)
 display=np.concatenate([encoded(v[...,:3]) for v in (pixels,base,result)],axis=1)
 image=a.output/(stem+'-comparison.png');Image.fromarray(np.rint(np.clip(display,0,1)*255).astype(np.uint8)).save(image)
 records.append(dict(name=stem,source_sha256=sha(path),analysis_sha256=sha(a.source/'people-reviewed'/(stem+'.json')),tracks=tracks,people=people,max_luminance_error=float(dy),off_exact=True,scene_sha256=sha(scene_path),result_sha256=sha(a.output/(stem+'-individual.rgba32f')),comparison_sha256=sha(image)))
 print(stem,'tracks',tracks,'samples',len(people),flush=True)
# Re-render in reverse request order from frozen explicit jobs, not accumulated history.
for record in reversed(records):
 job_path=a.output/(record['name']+'-individual.json');job=json.loads(job_path.read_text());target=a.output/(record['name']+'-reverse.rgba32f');job['output']=str(target.resolve());reverse_job=a.output/(record['name']+'-reverse.json');reverse_job.write_text(json.dumps(job));subprocess.run([str(a.renderer.resolve()),str(reverse_job.resolve())],check=True,capture_output=True,timeout=60)
 if sha(target)!=record['result_sha256']:raise ValueError('Reverse request mismatch')
 record['reverse_exact']=True
(a.output/'evidence.json').write_text(json.dumps(dict(cases=records,cut_before=cuts,source_evidence_sha256=sha(a.source/'combined/evidence.json'),renderer_sha256=sha(a.renderer),lut_sha256=sha(a.lut),runner_sha256=sha(__file__),association_sha256=sha(Path(__file__).with_name('person_tone_analysis.py')),scope='Automatic face skin plus uniquely face-associated pose AND body-skin material, bounded geometry/color association; not biometric identity, not exhaustive body coverage; perceptual flicker not certified'),indent=2))
