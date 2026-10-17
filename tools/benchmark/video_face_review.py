"""Inspect per-frame face-centered crops; not full-frame motion acceptance."""
import argparse,json,hashlib
from pathlib import Path
from PIL import Image,ImageDraw,ImageOps
import numpy as np
p=argparse.ArgumentParser();p.add_argument('job',type=Path);p.add_argument('output',type=Path);p.add_argument('--comparison',default='comparison-multi-reviewed');a=p.parse_args();a.output.mkdir(exist_ok=False)
root=a.job/'people-video-reviewed';render=root/a.comparison;manifest=json.loads((root/'manifest.json').read_text());e=json.loads((render/'evidence.json').read_text());sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
if sha(root/'manifest.json')!=e['manifest_sha256']:raise ValueError('Changed cases')
for c in manifest['cases']:
 name=c['name'];binding=json.loads((a.job/'people-reviewed'/(name+'.json')).read_text())
 if sha(Path(c['people_path']))!=c['people_sha256'] or binding['source_sha256']!=c['sha256']:raise ValueError('Changed face source')
 faces=sorted(binding['raw_face_details']['faces'],key=lambda f:f['box'][0]);board=Image.new('RGB',(720,780),(18,18,18));d=ImageDraw.Draw(board)
 d.text((8,5),f'Auto face-centered crops / frame {c["frame"]} / 12fps',fill='white')
 for row,f in enumerate(faces[:3]):
  x0,y0,x1,y1=f['box'];pad=.15*(y1-y0);box=(int(x0-pad),int(y0-pad),int(x1+pad),int(y1+pad))
  for col,mode in enumerate(['baseline','hybrid']):
   path=render/(name+'-'+mode+'.png')
   if sha(path)!=e['output_sha256'][path.name]:raise ValueError('Changed render')
   crop=ImageOps.contain(Image.open(path).crop(box),(220,210));board.paste(crop,(col*240+(220-crop.width)//2,30+row*240+(210-crop.height)//2));d.text((col*240+4,244+row*240),mode,fill='white')
  path=render/(name+'-hybrid-mask.npy')
  if sha(path)!=e['output_sha256'][path.name]:raise ValueError('Changed mask')
  m=np.load(path,allow_pickle=False)[...,0];crop=ImageOps.contain(Image.fromarray(np.rint(m*255).astype(np.uint8)).crop(box),(220,210));board.paste(crop.convert('RGB'),(480+(220-crop.width)//2,30+row*240+(210-crop.height)//2));d.text((484,244+row*240),'skin mask',fill='white')
 d.text((8,760),'Tears of Steel / Blender Foundation and Institute / CC BY3.0 / QA',fill='white');board.save(a.output/f'face-{c["index"]:03}.png')
