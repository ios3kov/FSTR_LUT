import argparse,json,hashlib,time
from pathlib import Path
import numpy as np
from PIL import Image
from video_anchor_analysis import warp_anchor,cut_between
p=argparse.ArgumentParser();p.add_argument('trial',type=Path);p.add_argument('--output',default='flow');p.add_argument('--strategy',choices=['direct','neighbor'],default='direct');a=p.parse_args();out=a.trial/a.output;out.mkdir(exist_ok=False);m=json.loads((a.trial/'manifest.json').read_text());sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();images=[]
for c in m['cases']:
 if sha(Path(c['path']))!=c['sha256']:raise ValueError('Changed source')
 images.append(np.asarray(Image.open(c['path']),dtype=np.float32)/255)
records=[];start=time.perf_counter()
for anchor in m['anchors']:
 d=anchor['direction'];i=anchor['index'];path=a.trial/(d+'-seed.npy')
 if sha(path)!=anchor['seed_sha256']:raise ValueError('Changed seed')
 mask=np.load(path,allow_pickle=False)
 if mask.ndim==3:mask=mask.max(0)
 previous=i;previous_mask=mask.astype(np.float32);previous_trust=np.ones(mask.shape,np.float32)
 indices=range(i,len(images)) if d=='forward' else range(i,-1,-1)
 for k in indices:
  rgb=images[k]
  if (d=='forward' and k<i) or (d=='backward' and k>i):continue
  if a.strategy=='direct':prob,trust=warp_anchor(rgb,images[i],mask)
  else:
   prob,trust=warp_anchor(rgb,images[previous],previous_mask)
   carried,_=warp_anchor(rgb,images[previous],previous_trust)
   trust*=carried
  cut=cut_between(images,k,i)
  if cut:trust*=0
  previous=k;previous_mask=prob;previous_trust=trust
  np.save(out/f'{d}-{k:03}.npy',prob);np.save(out/f'{d}-{k:03}-trust.npy',trust);records.append(dict(direction=d,index=k,cut=cut,valid_mean=float(trust.mean())))
(out/'evidence.json').write_text(json.dumps(dict(strategy=a.strategy,manifest_sha256=sha(a.trial/'manifest.json'),output_sha256={x.name:sha(x) for x in out.glob('*.npy')},records=records,seconds=time.perf_counter()-start),indent=2))
