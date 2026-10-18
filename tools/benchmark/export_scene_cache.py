"""Export an immutable development-only native analysis cache, never frames/weights.

Requires exact trial/case identities. Models/inference are not supplied by this bridge.
"""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('trial',type=Path);p.add_argument('cases',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest()
e=json.loads((a.trial/'evidence.json').read_text());cases={c['name']:c for c in json.loads(a.cases.read_text())['cases']}
# Analysis identity excludes selected LUT, strength, renderer and resulting colors.
policy=hashlib.sha256(json.dumps(dict(schema=1,association=e['association_sha256'],runner=e['runner_sha256'],support=e.get('support_evidence_sha256'),stable_face_strength=e.get('experimental_stable_face_strength',False),mean_face_field=e.get('experimental_mean_face_field',False),cuts=e.get('cut_before',[]),analysis_fields=[dict(source=r['source_sha256'],scene=r['scene_sha256'],people=[(x['mask_sha256'],x['samples_sha256'],x.get('toning_support_sha256'),x.get('toning_strength')) for x in r['people']]) for r in e['cases']]),sort_keys=True).encode()).hexdigest()
records=[];total=0
for record in e['cases']:
 c=cases[record['name']];path=Path(c['path'])
 if sha(path)!=c['sha256'] or c['sha256']!=record['source_sha256']:raise ValueError('Changed source pixels')
 rgb=np.array(Image.open(path).convert('RGB'),np.uint8);h,w=rgb.shape[:2];rgba=np.concatenate([rgb,np.full((h,w,1),255,np.uint8)],-1);height=float(c['source_height'])
 identity=f'FSTR_SCENE_V1\n{policy}\n{w} {h} {height:.17g}\n'.encode()+rgba.tobytes();key=hashlib.sha256(identity).hexdigest()
 scene=a.trial/(record['name']+'-scene.f32')
 if sha(scene)!=record['scene_sha256']:raise ValueError('Changed scene field')
 files=[(scene,key+'.weights',record['scene_sha256'])];people=[]
 if len(record['people'])>16:raise ValueError('Person budget')
 for index,person in enumerate(record['people']):
  mask,samples=Path(person['mask']),Path(person['samples']);count=samples.stat().st_size//16
  if count>32768 or samples.stat().st_size%16 or mask.stat().st_size!=h*w*4:raise ValueError('Person payload budget')
  if 'toning_support' in person:
   support=Path(person['toning_support']);files.append((support,f'{key}-p{index}.support',person['toning_support_sha256']))
  files.extend([(mask,f'{key}-p{index}.mask',person['mask_sha256']),(samples,f'{key}-p{index}.samples',person['samples_sha256'])]);people.append(dict(samples=count,mask_sha256=person['mask_sha256'],samples_sha256=person['samples_sha256']))
  if 'toning_support' in person:people[-1].update(toning_support_sha256=person['toning_support_sha256'],toning_strength=person['toning_strength'])
 size=sum(path.stat().st_size for path,_,_ in files);total+=size
 if size>64*1024*1024 or total>512*1024*1024:raise ValueError('Development cache export budget')
 for source,name,expected in files:
  if sha(source)!=expected:raise ValueError('Changed analysis payload')
  target=a.output/name
  if target.exists():
   if sha(target)!=expected:raise ValueError('Inconsistent duplicate source identity')
  else:target.write_bytes(source.read_bytes())
 header=dict(version=2 if any('toning_support_sha256' in p for p in people) else 1,key=key,policy=policy,width=w,height=h,source_height=height,scene_sha256=record['scene_sha256'],people=people)
 ready=a.output/(key+'.fstrscene');temporary=a.output/(key+'.pending');temporary.write_text(json.dumps(header));temporary.replace(ready);records.append(dict(name=record['name'],key=key,bytes=size))
(a.output/'export.json').write_text(json.dumps(dict(policy=policy,trial_sha256=sha(a.trial/'evidence.json'),bytes=total,records=records,scope='Read-only native development bridge; exact prepared RGBA8 input and source-height required; no model backend'),indent=2));print('Policy',policy,'frames',len(records),'bytes',total)
