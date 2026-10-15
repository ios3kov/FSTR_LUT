"""Fuse verified cached scene/people evidence and rerender the native look.

Caches are current-frame observations, never recursively stabilized masks.
"""
import argparse,hashlib,json,subprocess
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
from scene_analysis import stabilize_window,validate_weights
from scene_people import fuse_people
from scene_trial import decode,encode
from face_skin_analysis import FACE_SHA,DETECTOR_SHA
from person_head_analysis import POSE_SHA
from smart_analysis import MODEL_SHA

p=argparse.ArgumentParser()
for key in ('scene','people','cases','renderer','lut','output'):p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base=json.loads((a.scene/'evidence.json').read_text());entries={r['name']:r for r in base['cases']}
if base['renderer_sha256']!=sha(a.renderer) or base['lut_sha256']!=sha(a.lut):raise ValueError('Different native look')
images={};raw={};records=[];manifest=json.loads(a.cases.read_text())
base_cases={c['name']:c for c in base['sources']['cases']}
if len(manifest['cases'])!=len(base_cases) or {c['name'] for c in manifest['cases']}!=set(base_cases):raise ValueError('Different case set')
cache_binding=json.loads((a.scene/'scene-cache-binding.json').read_text())
if cache_binding['base_evidence_sha256']!=sha(a.scene/'evidence.json'):raise ValueError('Changed scene evidence')
for case in manifest['cases']:
 name=case['name'];path=Path(case['path'])
 if case['sha256']!=base_cases[name]['sha256'] or case.get('frame')!=base_cases[name].get('frame'):raise ValueError('Different case/frame')
 if Path(name).name!=name or sha(path)!=case['sha256']:raise ValueError('Changed source')
 im=Image.open(path)
 if im.mode not in ('RGB','L'):raise ValueError('Opaque input required')
 rgb=np.array(im.convert('RGB'),dtype=np.float32)/255;part=a.people/(name+'.npz');binding=json.loads((a.people/(name+'.json')).read_text())
 if binding['source_height']!=case['source_height']:raise ValueError('Different source scale')
 if binding['source_sha256']!=case['sha256'] or binding['evidence_sha256']!=sha(part) or binding['parts']!=entries[name]['person_parts'] or binding['models']!=dict(face=FACE_SHA,detector=DETECTOR_SHA,pose=POSE_SHA,skin=MODEL_SHA):raise ValueError('People provenance mismatch')
 scene_path=a.scene/(name+'-raw.npy');scene=np.load(scene_path,allow_pickle=False);validate_weights(scene,rgb.shape[:2])
 if sha(scene_path)!=cache_binding['raw_sha256'][name]:raise ValueError('Changed raw scene cache')
 with np.load(part,allow_pickle=False) as observation:weights=fuse_people(scene,observation['people'],observation['face'])
 images[name]=rgb;raw[name]=weights;np.save(a.output/(name+'-raw.npy'),weights)
 records.append(dict(name=name,scene_raw_sha256=sha(scene_path),people=binding,raw_sha256=sha(a.output/(name+'-raw.npy'))))
index={c['frame']:c['name'] for c in manifest['cases'] if 'frame' in c}
for case,record in zip(manifest['cases'],records):
 name=case['name'];rgb=images[name];h,w=rgb.shape[:2];window={0:(rgb,raw[name])}
 if 'frame' in case:
  for d in (-2,-1,1,2):
   other=index.get(case['frame']+d)
   if other:window[d]=(images[other],raw[other])
 weights=stabilize_window(window)
 if not np.array_equal(weights,stabilize_window(dict(reversed(list(window.items()))))):raise RuntimeError('Order dependence')
 np.save(a.output/(name+'-stable.npy'),weights)
 np.concatenate([decode(rgb),np.ones((h,w,1),np.float32)],-1).astype('<f4').tofile(a.output/'input.rgba32f')
 result=[]
 for mode,mask in [('off',weights*0),('on',weights)]:
  mask.astype('<f4').tofile(a.output/'weights.f32');out=a.output/(name+'-'+mode+'.rgba32f')
  subprocess.run([str(a.renderer.resolve()),str(a.lut.resolve()),str(w),str(h),str(a.output/'input.rgba32f'),str(a.output/'weights.f32'),str(out)],check=True)
  rendered=np.fromfile(out,dtype='<f4').reshape(h,w,4);result.append(rendered)
  Image.fromarray(np.rint(np.clip(encode(rendered[...,:3]),0,1)*255).astype(np.uint8)).save(a.output/(name+'-'+mode+'.png'))
 if sha(a.output/(name+'-off.rgba32f'))!=sha(a.scene/(name+'-off.rgba32f')):raise RuntimeError('Off regression')
 record.update(reverse_exact=True,off_exact=True,skin_mass=float(weights[...,0].mean()),max_luminance_delta=float(np.abs((result[1][...,:3]-result[0][...,:3])@np.array([.2126,.7152,.0722])).max()))
 board=Image.new('RGB',(w*4,h+28),(18,18,18));draw=ImageDraw.Draw(board)
 for k,(im,title) in enumerate([(Image.fromarray(np.rint(rgb*255).astype(np.uint8)),'Input'),(Image.open(a.scene/(name+'-on.png')),'Scene only'),(Image.open(a.output/(name+'-on.png')),'Scene + people'),(Image.fromarray(np.rint(weights[...,0]*255).astype(np.uint8)).convert('RGB'),'Skin weight')]):
  board.paste(im,(k*w,28));draw.text((k*w+5,8),title,fill='white')
 board.save(a.output/(name+'-review.jpg'),quality=95);print(name,flush=True)
(a.output/'evidence.json').write_text(json.dumps(dict(base_evidence_sha256=sha(a.scene/'evidence.json'),cache_binding_sha256=sha(a.scene/'scene-cache-binding.json'),post_run_capture_without_producer_hash=cache_binding['post_run_capture_without_producer_hash'],renderer_sha256=sha(a.renderer),lut_sha256=sha(a.lut),sources=manifest,cases=records,limitations='Research only. Four-pose cap; independent faces up to16. Texture-based detail confidence is not calibrated physical blur. No pixel truth or continuous perceptual approval.'),indent=2))
