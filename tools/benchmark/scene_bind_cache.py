"""Validate and bind an owned scene trial before a separate fusion experiment.

Older trials did not emit raw hashes. For these, this is explicitly a post-run
artifact capture, not a claim of producer-time integrity verification.
"""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from PIL import Image
from scene_analysis import validate_weights
p=argparse.ArgumentParser();p.add_argument('trial',type=Path);a=p.parse_args()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
evidence=a.trial/'evidence.json';base=json.loads(evidence.read_text());records={r['name']:r for r in base['cases']};hashes={};captured=[]
for c in base['sources']['cases']:
 name=c['name'];path=Path(c['path'])
 if Path(name).name!=name or sha(path)!=c['sha256']:raise ValueError('Invalid source')
 with Image.open(path) as im:w,h=im.size
 raw=a.trial/(name+'-raw.npy');validate_weights(np.load(raw,allow_pickle=False),(h,w));digest=sha(raw)
 if 'raw_sha256' in records[name]:
  if records[name]['raw_sha256']!=digest:raise ValueError('Changed producer raw bytes')
 else:captured.append(name)
 hashes[name]=digest
out=a.trial/'scene-cache-binding.json'
with out.open('x') as f:json.dump(dict(base_evidence_sha256=sha(evidence),raw_sha256=hashes,post_run_capture_without_producer_hash=captured),f,indent=2)
print(len(hashes),'raw fields verified; legacy post-run captures',len(captured))
