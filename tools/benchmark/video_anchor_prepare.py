import argparse,hashlib,json,shutil
from pathlib import Path
import numpy as np
from PIL import Image
from video_anchor_analysis import choose_anchor
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(exist_ok=False);(a.output/'frames').mkdir()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
base=json.loads((a.source/'combined-reviewed/evidence.json').read_text());lookup={r['name']:r for r in base['cases']};cases={c['name']:c for c in base['sources']['cases']};images=[];raw=[];records=[]
for i,f in enumerate(range(140,156)):
 name=f'motion-{f:03}';c=cases[name];path=Path(c['path']);rp=a.source/'combined-reviewed'/(name+'-raw.npy');pp=a.source/'people-reviewed'/(name+'.npz')
 if sha(path)!=c['sha256'] or sha(rp)!=lookup[name]['raw_sha256'] or sha(pp)!=lookup[name]['people']['evidence_sha256']:raise ValueError('Changed source/cache')
 rgb=np.asarray(Image.open(path),dtype=np.float32)/255;images.append(rgb);raw.append(np.load(rp,allow_pickle=False))
 # Official SAM2 directory loader selects jpg names but Pillow decodes PNG
 # content. Preserve exact lossless source bytes, no JPEG recompression.
 target=a.output/'frames'/f'{i:05}.jpg';shutil.copyfile(path,target)
 records.append(dict(index=i,frame=f,name=name,path=str(path.resolve()),sha256=sha(path),raw_path=str(rp.resolve()),raw_sha256=sha(rp),people_path=str(pp.resolve()),people_sha256=sha(pp)))
anchors=[]
for key,indices in [('forward',range(4)),('backward',range(12,16))]:
 detail,mask=choose_anchor(images,raw,indices);np.save(a.output/(key+'-seed.npy'),mask);detail['direction']=key;detail['seed_sha256']=sha(a.output/(key+'-seed.npy'));anchors.append(detail)
(a.output/'manifest.json').write_text(json.dumps(dict(cases=records,anchors=anchors,loader='Lossless PNG bytes with numeric jpg names; official Pillow loader; pixels unchanged',source_evidence_sha256=sha(a.source/'combined-reviewed/evidence.json')),indent=2));print(anchors)
