"""Execute automatic scene analysis, bounded temporal refinement and native grade.

Research evaluation only. Sources/rights and assumptions are supplied by cases.json.
"""
import argparse,json,hashlib,subprocess,time
from pathlib import Path
import cv2
import numpy as np
from PIL import Image,ImageDraw
from scene_analysis import SceneSegmenter,stabilize_window,ROLES
from smart_analysis import MODEL_SHA,LABELS

def decode(x):return np.where(x<=.04045,x/12.92,((x+.055)/1.055)**2.4)
def encode(x):return np.where(x<=.0031308,x*12.92,1.055*np.maximum(x,0)**(1/2.4)-.055)
def main():
 p=argparse.ArgumentParser()
 for k in ('cases','model','parts','renderer','lut','output'):p.add_argument('--'+k,type=Path,required=True)
 a=p.parse_args();a.output.mkdir(exist_ok=False);manifest=json.loads(a.cases.read_text());engine=SceneSegmenter(a.model)
 images={};raw={};records=[];labels={}
 for case in manifest['cases']:
  name=case['name']
  if Path(name).name!=name:raise ValueError('Unsafe case name')
  path=Path(case['path']);im=Image.open(path)
  if im.mode not in ('RGB','L'):raise ValueError('Opaque RGB/L input required')
  rgb=np.array(im.convert('RGB'),dtype=np.float32)/255
  if hashlib.sha256(path.read_bytes()).hexdigest()!=case['sha256']:raise ValueError('Changed source')
  part_path=a.parts/(name+'.npy');binding=json.loads((a.parts/(name+'.json')).read_text())
  if binding['source_sha256']!=case['sha256'] or binding['parts_sha256']!=hashlib.sha256(part_path.read_bytes()).hexdigest() or binding['model_sha256']!=MODEL_SHA or binding['labels']!=LABELS:raise ValueError('Person parts provenance mismatch')
  part=np.load(part_path,allow_pickle=False)
  start=time.perf_counter();weights,label=engine.analyze(rgb,part)
  images[name]=rgb;raw[name]=weights;labels[name]=label
  records.append(dict(name=name,person_parts=binding,seconds=time.perf_counter()-start,role_mass=dict(zip(ROLES,weights.mean((0,1)).astype(float))),unknown=float(1-weights.sum(-1).mean())))
  np.save(a.output/(name+'-raw.npy'),weights);print(name,records[-1]['seconds'],flush=True)
  records[-1]['raw_sha256']=hashlib.sha256((a.output/(name+'-raw.npy')).read_bytes()).hexdigest()
  Image.fromarray(label).save(a.output/(name+'-labels.png'))
 index={c['frame']:c['name'] for c in manifest['cases'] if 'frame' in c}
 palette=np.array([[1,.5,.35],[.5,.3,.7],[.8,.2,.6],[.15,.6,1],[.2,.9,.2],[.1,.8,.8],[.8,.8,.4],[1,1,1]],np.float32)
 temporal=[]
 for case,record in zip(manifest['cases'],records):
  name=case['name'];rgb=images[name];h,w=rgb.shape[:2];window={0:(rgb,raw[name])}
  if 'frame' in case:
   for d in (-2,-1,1,2):
    other=index.get(case['frame']+d)
    if other:window[d]=(images[other],raw[other])
  mask=stabilize_window(window);reverse=stabilize_window(dict(reversed(list(window.items()))))
  if not np.array_equal(mask,reverse):raise RuntimeError('Order dependence')
  np.save(a.output/(name+'-stable.npy'),mask)
  source=np.concatenate([decode(rgb),np.ones((h,w,1),np.float32)],-1).astype('<f4');source.tofile(a.output/'input.rgba32f')
  outputs=[]
  for mode,m in [('off',mask*0),('on',mask)]:
   m.astype('<f4').tofile(a.output/'weights.f32');out=a.output/(name+'-'+mode+'.rgba32f')
   subprocess.run([str(a.renderer.resolve()),str(a.lut.resolve()),str(w),str(h),str(a.output/'input.rgba32f'),str(a.output/'weights.f32'),str(out)],check=True)
   rendered=np.fromfile(out,dtype='<f4').reshape(h,w,4);outputs.append(rendered)
   im=Image.fromarray(np.rint(np.clip(encode(rendered[...,:3]),0,1)*255).astype(np.uint8));im.save(a.output/(name+'-'+mode+'.png'))
  delta=outputs[1][...,:3]-outputs[0][...,:3]
  record['max_linear_luminance_delta']=float(np.abs(delta@np.array([.2126,.7152,.0722])).max())
  record['mean_linear_rgb_delta']=float(np.abs(delta).mean());record['stable_mass']=dict(zip(ROLES,mask.mean((0,1)).astype(float)))
  record['reverse_exact']=True
  colored=mask@palette+rgb*(1-mask.sum(-1,keepdims=True));overlay=.45*rgb+.55*colored
  panel=Image.new('RGB',(w*4,h+28),(20,20,20));draw=ImageDraw.Draw(panel)
  for k,(im,title) in enumerate([(Image.fromarray(np.rint(rgb*255).astype(np.uint8)),'Input'),(Image.open(a.output/(name+'-off.png')),'LUT'),(Image.open(a.output/(name+'-on.png')),'Scene Smart prototype'),(Image.fromarray(np.rint(np.clip(overlay,0,1)*255).astype(np.uint8)),'Soft role overlay')]):
   panel.paste(im,(k*w,28));draw.text((k*w+5,8),title,fill='white')
  panel.save(a.output/(name+'-review.jpg'),quality=93)
  if 'frame' in case:temporal.append((case['frame'],np.concatenate([np.asarray(Image.open(a.output/(name+'-off.png'))),np.asarray(Image.open(a.output/(name+'-on.png')))],1)))
 if temporal:
  temporal.sort(key=lambda x:x[0]);shape=temporal[0][1].shape;writer=cv2.VideoWriter(str(a.output/'comparison.mp4'),cv2.VideoWriter_fourcc(*'mp4v'),25,(shape[1],shape[0]))
  if not writer.isOpened():raise RuntimeError('Video writer unavailable')
  for _,v in temporal:writer.write(cv2.cvtColor(v,cv2.COLOR_RGB2BGR))
  writer.release()
 (a.output/'evidence.json').write_text(json.dumps(dict(renderer_sha256=hashlib.sha256(a.renderer.read_bytes()).hexdigest(),lut_sha256=hashlib.sha256(a.lut.read_bytes()).hexdigest(),loading_info=engine.loading_info,cases=records,sources=manifest,model=json.loads((a.model/'provenance.json').read_text()),limitations='Research only, no semantic ground truth. Raw softmax is not calibrated confidence. Automated motion/visual quality not certified.'),indent=2))
if __name__=='__main__':main()
