"""Compare isolated AE PSDs to explicit linear-float native references.

Declared calibration only: 480x200 opaque unmanaged sRGB, Photoshop8-bit export.
Tolerance fixed at2code values before observing host output. No perceptual verdict.
"""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('host',type=Path);p.add_argument('native',type=Path);p.add_argument('baseline',type=Path);a=p.parse_args()
sha=lambda path:hashlib.sha256(Path(path).read_bytes()).hexdigest()
report=(a.host/'host-report.txt').read_text()
if 'PASS scalar bridge' not in report:raise ValueError('Host scenario incomplete')
def image(label,index=0):
 files=list(a.host.glob(f'{label}_*.psd'))
 if len(files)<=index:raise ValueError('Missing host output '+label)
 path=sorted(files)[index];pixels=np.asarray(Image.open(path).convert('RGB'))
 if pixels.shape!=(200,480,3):raise ValueError('Quarter source calibration mismatch')
 return pixels,path
rows=[]
def compare(label,reference,index=0):
 actual,path=image(label,index);v=np.fromfile(reference,np.float32).reshape(200,480,4)[...,:3];enc=np.where(v<=.0031308,v*12.92,1.055*np.maximum(v,0)**(1/2.4)-.055);expected=np.rint(np.clip(enc,0,1)*255).astype(np.uint8);delta=np.abs(actual.astype(int)-expected.astype(int));maximum=int(delta.max());rows.append(dict(label=label,index=index,max_code_error=maximum,mean_code_error=float(delta.mean()),passed=maximum<=2,output_sha256=sha(path),reference_sha256=sha(reference)))
compare('01_off',a.baseline/'people-001-ordinary.rgba32f');compare('02_on',a.native/'people-001.rgba32f');compare('04_on',a.native/'people-001.rgba32f')
for label in ('05_look','06_strength'):compare(label,a.host/(label+'-expected.rgba32f'))
for i in range(36):
 compare('09_sequence',a.native/f'people-{i+1:03}.rgba32f',i);compare('10_sequence_off',a.baseline/f'people-{i+1:03}-ordinary.rgba32f',i)
pairs=[]
for left,right in [('01_off','03_off'),('02_on','04_on'),('07_upstream_on','08_upstream_off')]:
 l,_=image(left);r,_=image(right);delta=int(np.abs(l.astype(int)-r.astype(int)).max());pairs.append(dict(left=left,right=right,max_code_error=delta,passed=delta<=1))
e=dict(comparisons=rows,pairs=pairs,passed=all(r['passed'] for r in rows+pairs),limitation='Photoshop8bit transport; no continuous perceptual or full-resolution model acceptance')
(a.host/'image-verification.json').write_text(json.dumps(e,indent=2));print('PASS' if e['passed'] else 'FAIL','native image comparisons',len(rows),'pairs',len(pairs));raise SystemExit(0 if e['passed'] else 1)
