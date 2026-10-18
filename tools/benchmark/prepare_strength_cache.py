"""Convert a bound scalar trial to raw analysis tensors for the native bridge."""
import argparse,hashlib,json,shutil
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser()
for k in ('baseline','strength','source','support','output'):p.add_argument('--'+k,type=Path,required=True)
a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
e=json.loads((a.baseline/'evidence.json').read_text());s=json.loads((a.strength/'evidence.json').read_text())
if s['reference_source_sha256']!=sha(a.baseline/'evidence.json'):raise ValueError('Different baseline')
if [r['name'] for r in e['cases']]!=[r['name'] for r in s['cases']]:raise ValueError('Different sequence')
for record,strength in zip(e['cases'],s['cases']):
 name=record['name'];binding_path=a.source/'people-reviewed'/(name+'.json')
 if sha(binding_path)!=record['analysis_sha256']:raise ValueError('Changed people evidence')
 binding=json.loads(binding_path.read_text());details=sorted(binding['raw_face_details']['faces'],key=lambda f:tuple(f['box']))
 if len(details)!=len(record['tracks']):raise ValueError('Ambiguous track mapping')
 sb=json.loads((a.support/(name+'.json')).read_text());sp=a.support/(name+'.npz')
 if sb['source_sha256']!=record['source_sha256'] or sha(sp)!=sb['evidence_sha256']:raise ValueError('Changed support')
 with np.load(sp,allow_pickle=False) as f:g=f['raw_face_support'].copy()
 h,w=g.shape;parts_path=a.source/'parts'/(name+'.npy')
 if sha(parts_path)!=binding['parts']['parts_sha256']:raise ValueError('Changed material evidence')
 parts=np.load(parts_path,allow_pickle=False);non_skin=np.clip((parts[...,0]+parts[...,1]+parts[...,4]-.6)/.3,0,1);veto=1-non_skin*non_skin*(3-2*non_skin)
 by_track={v['track']:v for v in record['people']};active={v['track']:v for v in strength['anchors_used']}
 for face,t in zip(details,record['tracks']):
  if t not in by_track or t not in active:continue
  x0,y0,x1,y1=face['box'];x0=max(0,int(np.floor(x0)));y0=max(0,int(np.floor(y0)));x1=min(w,int(np.ceil(x1)));y1=min(h,int(np.ceil(y1)))
  field=np.zeros_like(g);field[y0:y1,x0:x1]=g[y0:y1,x0:x1]*veto[y0:y1,x0:x1]
  path=a.output/(name+f'-person-{t}.support');field.astype('<f4').tofile(path)
  by_track[t].update(toning_support=str(path.resolve()),toning_support_sha256=sha(path),toning_strength=active[t]['strength'])
 scene=a.baseline/(name+'-scene.f32')
 if sha(scene)!=record['scene_sha256']:raise ValueError('Changed scene evidence')
 shutil.copyfile(scene,a.output/(name+'-scene.f32'))
e['scalar_algorithm_sha256']=s['reference_algorithm_sha256'];e['scalar_evidence_sha256']=sha(a.strength/'evidence.json')
(a.output/'evidence.json').write_text(json.dumps(e,indent=2))
(a.output/'cases.json').write_text(json.dumps(json.loads((a.source/'combined/evidence.json').read_text())['sources']))
