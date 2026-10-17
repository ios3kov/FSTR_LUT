"""Automatic per-face seeds for a bounded multi-person skin-class trial.

Object IDs aid geometry only; no identity recognition or hand-seed claim.
"""
import argparse,json,hashlib,shutil
from pathlib import Path
import numpy as np
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(exist_ok=False);(a.output/'frames').mkdir()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
e=json.loads((a.source/'combined/evidence.json').read_text());expected={r['name']:r for r in e['cases']};records=[];candidates=[]
if len(e['sources']['cases'])<6:raise ValueError('At least six frames required')
for i,c in enumerate(e['sources']['cases']):
 name=c['name'];path=Path(c['path']);raw=a.source/'combined'/(name+'-raw.npy');people=a.source/'people-reviewed'/(name+'.npz');binding=json.loads((a.source/'people-reviewed'/(name+'.json')).read_text())
 if binding!=expected[name]['people'] or binding['source_sha256']!=c['sha256'] or sha(path)!=c['sha256'] or sha(people)!=binding['evidence_sha256'] or sha(raw)!=expected[name]['raw_sha256']:raise ValueError('Changed source evidence')
 with np.load(people,allow_pickle=False) as f:face=f['raw_face_mask']
 masks=[]
 for r in sorted(binding['raw_face_details']['faces'],key=lambda r:r['box'][0]):
  if r['status']!='confirmed' or r['detector_score']<.8:continue
  x0,y0=np.floor(r['box'][:2]).astype(int);x1,y1=np.ceil(r['box'][2:]).astype(int);m=np.zeros(face.shape,bool);m[max(0,y0):min(face.shape[0],y1),max(0,x0):min(face.shape[1],x1)]=face[max(0,y0):min(face.shape[0],y1),max(0,x0):min(face.shape[1],x1)]>.3
  if m.sum()>=24:masks.append(m)
 candidates.append(masks);shutil.copyfile(path,a.output/'frames'/f'{i:05}.jpg')
 records.append(dict(index=i,frame=c['frame'],name=name,path=str(path),sha256=sha(path),raw_path=str(raw.resolve()),raw_sha256=sha(raw),people_path=str(people.resolve()),people_sha256=sha(people)))
anchors=[]
for direction,indices in [('forward',range(3)),('backward',range(len(records)-3,len(records)))]:
 i=max(indices,key=lambda j:(len(candidates[j]),min((int(m.sum()) for m in candidates[j]),default=0),-j))
 if len(candidates[i])<2:raise ValueError('No automatic multi-person anchor')
 masks=np.stack(candidates[i]);target=a.output/(direction+'-seed.npy');np.save(target,masks)
 anchors.append(dict(direction=direction,index=i,objects=len(masks),amplitude=float(np.median(np.load(records[i]['raw_path'],allow_pickle=False)[...,0][masks.any(0)])),seed_sha256=sha(target),selection='confirmed current-face skin masks; maximize face count then smallest mask area; no cross-person identity claim'))
if anchors[0]['objects']!=anchors[1]['objects']:raise ValueError('Different endpoint object count')
(a.output/'manifest.json').write_text(json.dumps(dict(cases=records,anchors=anchors,loader='Original lossless PNG bytes under numeric jpg filenames',source_evidence_sha256=sha(a.source/'combined/evidence.json'),scope='Union of confirmed face skin tracks; not all body skin, crowds or identity matching'),indent=2));print(anchors)
