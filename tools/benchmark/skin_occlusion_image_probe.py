"""C2098-specific planted occluder counterexample; not real-hand validation."""
import sys,json,subprocess,argparse,hashlib
from pathlib import Path
import numpy as np,cv2,torch
sys.path.insert(0,str(Path(__file__).resolve().parent))
from visible_face_analysis import VisibleFaceSegmenter,limit_mask
from blur_mask_recovery import recover_blur
from smart_analysis import encode_srgb
from PIL import Image,ImageDraw
parser=argparse.ArgumentParser()
for name in ('source','baseline','weights','output'):parser.add_argument('--'+name,type=Path,required=True)
args=parser.parse_args();out=args.output;out.mkdir(exist_ok=False,parents=True)
torch.set_num_threads(2);cv2.setNumThreads(1)
model=VisibleFaceSegmenter(args.weights)
r=json.loads((args.baseline/'verification.json').read_text())
from visible_face_trial import validate_baseline
validate_baseline(r)
with args.source.open('rb') as f:source_sha=hashlib.file_digest(f,'sha256').hexdigest()
if source_sha!=r['source_sha256']:raise ValueError('This fixture requires the exact C2098 baseline')
raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(args.source),'-an','-vf',"fps=25,scale=720:1280:flags=lanczos:in_color_matrix=bt709:out_range=full,select='between(n,53,57)',setpts=N/(25*TB)",'-frames:v','5','-pix_fmt','rgb24','-f','rawvideo','pipe:1'])
if len(raw)!=5*1280*720*3:raise ValueError('Incomplete selected frames')
images=encode_srgb((np.frombuffer(raw,np.uint8).reshape(5,1280,720,3).astype(np.float32)/255)**2.4)
base=np.memmap(args.baseline/'stable-mask.f32',np.float32,'r',shape=(156,640,360))[53:58]
results=[];boards=[]
for kind in ('control','flat_skin','texture_skin'):
 rgb=images.copy();region=np.zeros((1280,720),np.float32);region[549:577,380:400]=1
 if kind=='flat_skin':rgb[2,549:577,380:400]=np.median(images[1,564:584,384:401],axis=(0,1))
 if kind=='texture_skin':rgb[2,549:577,380:400]=cv2.resize(images[1,564:584,384:401],(20,28))
 low=[cv2.resize(x,(360,640),interpolation=cv2.INTER_AREA) for x in rgb];gates=[]
 for k,x in enumerate(rgb):
  v=model.analyze(x,[f['box'] for f in r['details'][k+53]['faces']]);gates.append(limit_mask(base[k],cv2.resize(v,(360,640),interpolation=cv2.INTER_AREA)))
 final,info=recover_blur(low,base,gates,2,[False]*4);roi=(slice(276,287),slice(191,199))
 results.append(dict(kind=kind,gate_inside_mean=float(gates[2][roi].mean()),final_inside_mean=float(final[roi].mean()),prior_inside_mean=float(base[2][roi].mean()),gate_mass=float(gates[2].sum()),final_mass=float(final.sum()),info=info))
 np.savez_compressed(out/(kind+'.npz'),rgb=low[2],gate=gates[2],final=final,region=cv2.resize(region,(360,640),interpolation=cv2.INTER_AREA))
 overlay=lambda x,m:x*(1-.7*m[...,None])+np.array([.15,.95,.5])*.7*m[...,None]
 u8=lambda x:np.rint(np.clip(x,0,1)*255).astype(np.uint8)
 board=Image.new('RGB',(600,224),(18,20,24));draw=ImageDraw.Draw(board)
 for i,(name,img) in enumerate(zip(('source','visible gate','recovery'),(low[2],overlay(low[2],gates[2]),overlay(low[2],final)))):
  draw.text((i*200+3,3),kind+' / '+name,fill='white');board.paste(Image.fromarray(u8(img)).crop((166,251,216,301)).resize((200,200)),(i*200,24))
 boards.append(board)
board=Image.new('RGB',(600,672))
for i,b in enumerate(boards):board.paste(b,(0,i*224))
board.save(out/'comparison.png');(out/'report.json').write_text(json.dumps(results,indent=2));(out/'provenance.json').write_text(json.dumps(dict(source_sha256=source_sha,script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),frame_indices=[53,54,55,56,57],fixture='planted patch, not real object',resampling='sRGB area; differs from baseline linear-resolution path'),indent=2));print(json.dumps(results,indent=2))
