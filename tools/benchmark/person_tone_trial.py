"""Automatic face-confirmed individual-tone research trial, no manual targets.

Uses existing detector/skin evidence. Does not claim exposed hands or body identity.
"""
import argparse,hashlib,json,subprocess
from pathlib import Path
import numpy as np
from PIL import Image
from person_tone_analysis import observation,associate,ownership,attach_body_skin,stabilize_fields,mean_face_fields
from scene_analysis import stabilize_window
from scene_people import face_field
from video_anchor_analysis import cut_between
p=argparse.ArgumentParser()
for key in ('source','output','renderer','lut'):p.add_argument('--'+key,type=Path,required=True)
p.add_argument('--support',type=Path)
p.add_argument('--stable-face-strength',action='store_true')
p.add_argument('--mean-face-field',action='store_true')
p.add_argument('--sam-face-fields',type=Path)
p.add_argument('--cut-before',type=int,action='append',default=[])
a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest()
producer_files=[Path(__file__),Path(__file__).with_name('person_tone_analysis.py'),Path(__file__).with_name('scene_analysis.py'),Path(__file__).with_name('scene_people.py'),Path(__file__).with_name('video_anchor_analysis.py'),Path(__file__).with_name('person_sam_fields.py')]
producer_bindings={str(p):sha(p) for p in producer_files};renderer_binding=sha(a.renderer);lut_binding=sha(a.lut)
if sum([a.stable_face_strength,a.mean_face_field,bool(a.sam_face_fields)])>1:raise ValueError('Compare one experimental proposal at a time')
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
 geometry=None
 if a.support:
  support_path=a.support/(name+'.npz');support_binding=json.loads((a.support/(name+'.json')).read_text())
  if support_binding['source_sha256']!=case['sha256'] or support_binding['models']!=binding['models'] or sha(support_path)!=support_binding['evidence_sha256']:raise ValueError('Changed independent face support')
  with np.load(support_path,allow_pickle=False) as f:geometry=f['raw_face_support'].copy()
 parts_path=a.source/'parts'/(name+'.npy')
 if sha(parts_path)!=binding['parts']['parts_sha256'] or binding['parts']['source_sha256']!=case['sha256']:raise ValueError('Changed body-material evidence')
 parts=np.load(parts_path,allow_pickle=False)
 observations=[]
 for face in sorted(binding['raw_face_details']['faces'],key=lambda r:tuple(r['box'])):
  field,detail=face_field(rgb,mask,[face],case['source_height']/rgb.shape[0])
  if not detail:continue
  support=None
  if geometry is not None:
   x0,y0=np.floor(face['box'][:2]).astype(int);x1,y1=np.ceil(face['box'][2:]).astype(int);support=np.zeros_like(field);support[max(0,y0):min(field.shape[0],y1),max(0,x0):min(field.shape[1],x1)]=geometry[max(0,y0):min(field.shape[0],y1),max(0,x0):min(field.shape[1],x1)]
  if support is not None and np.count_nonzero(support>.25)<8:support=None
  o=observation(face['box'],field,linear,float(face['detector_score']),descriptor_support=support)
  if o is not None and support is not None:
   non_skin=np.clip((parts[...,0]+parts[...,1]+parts[...,4]-.6)/.3,0,1);veto=1-non_skin*non_skin*(3-2*non_skin);o['face_geometry']=support;o['face_ceiling']=support*veto;o['ceiling']=o['face_ceiling'].copy();o['recovery_ceiling']=veto.copy();o['recovery_ceiling'][max(0,y0):min(field.shape[0],y1),max(0,x0):min(field.shape[1],x1)]=o['ceiling'][max(0,y0):min(field.shape[0],y1),max(0,x0):min(field.shape[1],x1)]
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
if a.mean_face_field:render_frames=mean_face_fields(frames,render_frames,ids)
sam_evidence=None
if a.sam_face_fields:
 from person_sam_fields import apply_sam_faces
 render_frames,sam_evidence=apply_sam_faces(render_frames,ids,a.sam_face_fields,cases)
if a.stable_face_strength:
 for observations in render_frames:
  for o in observations:
   if 'face_geometry' not in o:continue
   t=np.clip((o['mask']-.02)/.28,0,1);shape=t*t*(3-2*t)*o['face_geometry'];ceiling=np.maximum(o.get('ceiling',o['mask']),o['mask']);o['mask']=np.minimum(ceiling,np.maximum(o['mask'],shape))
records=[]
for index,(obs,tracks,pixels,path) in enumerate(zip(render_frames,ids,rgba,sources)):
 h,w=pixels.shape[:2];stem=cases[index]['name'];input_path=a.output/(stem+'.rgba32f');pixels.tofile(input_path)
 active=[(o,t) for o,t in zip(obs,tracks) if t is not None and t in references]
 fields=ownership(np.stack([o['mask'] for o,t in active])) if active else np.empty((0,h,w),np.float32)
 scene_window={}
 for offset in (-2,-1,0,1,2):
  j=index+offset
  if 0<=j<len(cases) and not any(k in cuts for k in range(min(j,index)+1,max(j,index)+1)):scene_window[offset]=(rgb_images[j],scene_raw[j])
 scene=stabilize_window(scene_window)
 if len(fields) and (a.stable_face_strength or a.mean_face_field or a.sam_face_fields):
  requested=fields.sum(0)
  if a.mean_face_field or a.sam_face_fields:
   requested=np.zeros((h,w),np.float32);yy,xx=np.mgrid[:h,:w]
   for field,(o,t) in zip(fields,active):
    x0,y0,x1,y1=o['box'];requested+=field*((xx>=x0)&(xx<x1)&(yy>=y0)&(yy<y1))
  target=np.maximum(scene[...,0],np.minimum(1.,requested));remaining=np.maximum(0,1-target);scene[...,1:]*=np.minimum(1.,remaining/np.maximum(scene[...,1:].sum(-1),1e-12))[...,None];scene[...,0]=target
 if len(fields):
  proposal=fields.sum(0);unknown=np.maximum(0,1-scene.sum(-1));scene[...,0]+=np.minimum(unknown,np.maximum(0,proposal-scene[...,0]))
 scene_path=a.output/(stem+'-scene.f32');scene.astype(np.float32).tofile(scene_path)
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
if any(sha(Path(path))!=binding for path,binding in producer_bindings.items()) or sha(a.renderer)!=renderer_binding or sha(a.lut)!=lut_binding:raise ValueError('Producer/renderer changed during trial; evidence rejected')
(a.output/'evidence.json').write_text(json.dumps(dict(producer_bindings=producer_bindings,sam_evidence_sha256=sam_evidence,cases=records,cut_before=cuts,experimental_stable_face_strength=a.stable_face_strength,experimental_mean_face_field=a.mean_face_field,support_evidence_sha256=sha(a.support/'evidence.json') if a.support else None,source_evidence_sha256=sha(a.source/'combined/evidence.json'),renderer_sha256=sha(a.renderer),lut_sha256=sha(a.lut),runner_sha256=sha(__file__),association_sha256=sha(Path(__file__).with_name('person_tone_analysis.py')),scope='Automatic face skin plus uniquely face-associated pose AND body-skin material, bounded geometry/color association; not biometric identity, not exhaustive body coverage; perceptual flicker not certified'),indent=2))
