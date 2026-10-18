"""Fixed-shape local UperNet port. No model weights added to the repository."""
import argparse,hashlib,json,math
from pathlib import Path
import torch
import numpy as np
p=argparse.ArgumentParser();p.add_argument('--model',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(exist_ok=False)
from transformers import UperNetForSemanticSegmentation
source=json.loads((a.model/'provenance.json').read_text())
for name,sha in source['files'].items():
 if Path(name).name!=name or hashlib.sha256((a.model/name).read_bytes()).hexdigest()!=sha:raise ValueError('Changed model')
torch.set_num_threads(4)
model,info=UperNetForSemanticSegmentation.from_pretrained(a.model,local_files_only=True,output_loading_info=True)
if any(info[k] for k in ('missing_keys','unexpected_keys','mismatched_keys','error_msgs')):raise ValueError('Incomplete model')
model.eval()
# ONNX's exporter cannot lower adaptive average pools with non-divisible bins.
# For this fixed input, use the exact documented floor/ceil bin arithmetic.
class FixedPool(torch.nn.Module):
 def __init__(self,size):super().__init__();self.size=size if isinstance(size,tuple) else (size,size)
 def forward(self,x):
  h,w=x.shape[-2:];oh,ow=self.size
  return torch.cat([torch.cat([x[...,math.floor(i*h/oh):math.ceil((i+1)*h/oh),math.floor(j*w/ow):math.ceil((j+1)*w/ow)].mean((-2,-1),keepdim=True) for j in range(ow)],-1) for i in range(oh)],-2)
def replace(m):
 for name,child in list(m.named_children()):
  if isinstance(child,torch.nn.AdaptiveAvgPool2d):
   replacement=FixedPool(child.output_size);setattr(m,name,replacement)
   # UperNet keeps an additional Python list used by its forward method.
   if hasattr(m,"layers"):m.layers=[replacement if v is child else v for v in m.layers]
  else:replace(child)
class Wrapper(torch.nn.Module):
 def __init__(self,m):super().__init__();self.model=m
 def forward(self,x):return self.model(pixel_values=x).logits
x=torch.from_numpy(np.random.default_rng(18).normal(0,.3,(1,3,512,512)).astype('float32'))
with torch.inference_mode():expected=model(pixel_values=x).logits.numpy()
replace(model)
with torch.inference_mode():actual=model(pixel_values=x).logits.numpy()
err=float(np.abs(actual-expected).max());print('pool replacement max',err,flush=True)
if err>1e-4:raise ValueError('Pool substitution differs')
out=a.output/'scene.onnx'
torch.onnx.export(Wrapper(model),x,out,input_names=['normalized_srgb'],output_names=['logits'],opset_version=17,dynamo=False,do_constant_folding=True)
import onnxruntime as ort
options=ort.SessionOptions();options.intra_op_num_threads=4
session=ort.InferenceSession(str(out),sess_options=options,providers=['CPUExecutionProvider'])
pred=session.run(None,{'normalized_srgb':x.numpy()})[0];onnxerr=float(np.abs(pred-expected).max());print('ONNX max',onnxerr,flush=True)
if onnxerr>3e-4:raise ValueError('ONNX equivalence failed')
manifest={'schema':1,'source':source,'onnx_sha256':hashlib.sha256(out.read_bytes()).hexdigest(),'input':{'shape':[1,3,512,512],'layout':'NCHW','color':'sRGB Rec709 straight opaque [0,1]','mean':[.485,.456,.406],'std':[.229,.224,.225],'resize':'PIL bilinear uint8 RGB to512 square'},'output':{'shape':list(expected.shape),'layout':'NCHW logits; resize bilinear align_corners=False before softmax','id2label':model.config.id2label},'validation':{'pool_max':err,'onnx_max':onnxerr,'tolerance':3e-4},'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
(a.output/'manifest.json').write_text(json.dumps(manifest,indent=2))
