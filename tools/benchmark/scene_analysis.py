"""Whole-scene research: opaque sRGB, explicit bounded windows, no history.

Eight confidence-weighted roles; missing mass means ordinary LUT. Semantic
models are fallible. No person->skin substitution or per-frame auto white balance.
"""
from pathlib import Path
import hashlib
import json
import cv2
import numpy as np

ROLES = ('skin', 'hair', 'clothing', 'sky', 'vegetation', 'water', 'built', 'emissive')
GROUPS = {3: ('sky',), 4: ('tree', 'grass', 'plant', 'field', 'flower', 'palm'),
          5: ('water', 'sea', 'river', 'waterfall', 'lake', 'swimming pool'),
          6: ('wall', 'building', 'floor', 'ceiling', 'road', 'sidewalk', 'house',
              'skyscraper', 'bridge', 'path', 'stairs', 'fence'),
          7: ('lamp', 'light', 'chandelier', 'streetlight')}

def smooth(a,b,x):
    t=np.clip((x-a)/(b-a),0,1);return t*t*(3-2*t)

def validate_image(a):
    if a.ndim!=3 or a.shape[2]!=3 or min(a.shape[:2])<1 or max(a.shape[:2])>2048 or not np.isfinite(a).all() or a.min()<0 or a.max()>1:
        raise ValueError('Finite opaque sRGB RGB [0,1] up to2048 required')

def validate_weights(w,shape):
    if w.shape!=(*shape,8) or not np.isfinite(w).all() or w.min()<0 or w.max()>1 or np.max(w.sum(-1))>1.000001:
        raise ValueError('Invalid eight-role probability mass')

def information_support(rgb):
    validate_image(rgb)
    # A uniform field contains no reliable semantic structure. Keep the global
    # look even when the classifier confidently invents sky/person labels.
    return float(smooth(.005,.025,float(np.std(rgb,axis=(0,1),dtype=np.float64).max())))

def refine_weights(rgb,w):
    """Soft source-guided neighborhood; no binary segmentation edges."""
    validate_image(rgb);validate_weights(w,rgb.shape[:2]);h,ww=rgb.shape[:2]
    pad=np.pad(rgb,((2,2),(2,2),(0,0)),mode='edge');pw=np.pad(w,((2,2),(2,2),(0,0)),mode='edge')
    total=np.zeros_like(w);den=np.zeros((h,ww,1),np.float32)
    for dy in range(-2,3):
        for dx in range(-2,3):
            other=pad[2+dy:2+dy+h,2+dx:2+dx+ww]
            k=np.exp(-(dx*dx+dy*dy)/4-np.mean((rgb-other)**2,-1,keepdims=True)/.02)
            total+=pw[2+dy:2+dy+h,2+dx:2+dx+ww]*k;den+=k
    return (total/den).astype(np.float32)

class SceneSegmenter:
    def __init__(self,path):
        from transformers import SegformerImageProcessor,SegformerForSemanticSegmentation
        import torch
        self.torch=torch;torch.set_num_threads(4);path=Path(path)
        manifest=json.loads((path/'provenance.json').read_text())
        for name,digest in manifest['files'].items():
            if Path(name).name!=name or hashlib.sha256((path/name).read_bytes()).hexdigest()!=digest:raise ValueError('Model bytes changed')
        self.processor=SegformerImageProcessor.from_pretrained(path,local_files_only=True)
        self.model=SegformerForSemanticSegmentation.from_pretrained(path,local_files_only=True,use_safetensors=True).eval()
        self.labels={int(k):v.strip() for k,v in self.model.config.id2label.items()}
        if len(self.labels)!=150 or self.labels.get(12)!='person':raise ValueError('Unsupported scene label schema')
    def analyze(self,rgb,person_parts=None):
        validate_image(rgb);h,w=rgb.shape[:2];torch=self.torch
        data=self.processor(images=np.rint(rgb*255).astype(np.uint8),return_tensors='pt')
        with torch.inference_mode():
            logits=self.model(**data).logits
            probs=torch.nn.functional.interpolate(logits,size=(h,w),mode='bilinear',align_corners=False).softmax(1)[0].numpy()
        if not np.isfinite(probs).all():raise ValueError('Nonfinite model probabilities')
        peak=probs.max(0);confidence=smooth(.35,.8,peak);roles=np.zeros((h,w,8),np.float32)
        for role,names in GROUPS.items():roles[...,role]=probs[[i for i,name in self.labels.items() if name in names]].sum(0)
        if person_parts is not None:
            if person_parts.shape!=(h,w,6) or not np.isfinite(person_parts).all() or person_parts.min()<0 or person_parts.max()>1 or np.max(person_parts.sum(-1))>1.001:raise ValueError('Invalid person-part probabilities')
            # Require two independent semantic views. A face/hand color does not
            # by itself label architecture as skin; no nearest-face ownership.
            person=probs[12]
            roles[...,0]=person*(person_parts[...,2]+person_parts[...,3])
            roles[...,1]=person*person_parts[...,1]
            roles[...,2]=person*person_parts[...,4]
        roles*=confidence[...,None]*information_support(rgb)
        roles=refine_weights(rgb,roles)
        return roles,probs.argmax(0).astype(np.uint8)

def stabilize_window(frames,center=0):
    """frames maps offsets -2..2 to (rgb, raw weights); exact explicit context.

    Only immediate two-sided supported gaps may bridge. No recursion/stale masks.
    Occlusion/flow/appearance reject each borrowed sample; cuts stop propagation.
    """
    if center!=0 or 0 not in frames or any(k not in range(-2,3) for k in frames):raise ValueError('Explicit centered window required')
    image,current=frames[0];validate_image(image);validate_weights(current,image.shape[:2]);h,w=image.shape[:2]
    for rgb,m in frames.values():
        validate_image(rgb);validate_weights(m,image.shape[:2])
        if rgb.shape!=image.shape:raise ValueError('Window RGB geometry mismatch')
    gray=lambda rgb:np.rint(cv2.cvtColor(rgb,cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
    grid=np.stack(np.meshgrid(np.arange(w),np.arange(h)),-1).astype(np.float32);warped={}
    for k in sorted(frames):
        if k==0:continue
        if any(j not in frames for j in range(min(0,k),max(0,k)+1)):continue
        if any(np.abs(cv2.resize(frames[j][0],(32,32))-cv2.resize(frames[j+1][0],(32,32))).mean()>.10 for j in range(min(0,k),max(0,k))):continue
        other,m=frames[k];forward=cv2.calcOpticalFlowFarneback(gray(image),gray(other),None,.5,3,15,3,5,1.2,0)
        backward=cv2.calcOpticalFlowFarneback(gray(other),gray(image),None,.5,3,15,3,5,1.2,0)
        xy=grid+forward
        remap=lambda a:cv2.remap(a,xy[...,0],xy[...,1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
        valid=(xy[...,0]>=0)&(xy[...,0]<=w-1)&(xy[...,1]>=0)&(xy[...,1]<=h-1)
        fb=np.linalg.norm(forward+remap(backward),axis=-1)
        appearance=np.abs(image-remap(other)).mean(-1)
        support=valid*(1-smooth(.5,2.,fb))*(1-smooth(.025,.10,appearance))
        warped[k]=(remap(m),support[...,None])
    result=current.copy();den=np.ones((h,w,1),np.float32)
    # Current support prevents borrowing a confident different class wholesale.
    for k,(m,valid) in warped.items():
        agreement=1-smooth(.10,.40,np.max(np.abs(m-current),-1,keepdims=True))
        factor=.2*valid*agreement*smooth(.02,.10,current.sum(-1,keepdims=True))
        result+=factor*m;den+=factor
    result/=den
    if -1 in warped and 1 in warped:
        a,va=warped[-1];b,vb=warped[1]
        empty=1-smooth(.02,.10,current.sum(-1,keepdims=True))
        bridge=.5*np.minimum(a,b)*va*vb*empty
        result+=bridge*(1-result.sum(-1,keepdims=True))
    result=np.maximum(result,0);result/=np.maximum(1,result.sum(-1,keepdims=True))
    return result.astype(np.float32)
