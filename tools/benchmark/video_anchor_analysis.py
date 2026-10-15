"""Deterministic offline anchor proposals, never render-time tracker state."""
import cv2
import numpy as np
from scene_analysis import smooth,validate_image,validate_weights


def choose_anchor(images,raw,indices):
    candidates=[]
    for i in indices:
        rgb=images[i];validate_image(rgb);validate_weights(raw[i],rgb.shape[:2]);skin=raw[i][...,0]
        n,labels,stats,_=cv2.connectedComponentsWithStats((skin>.55).astype(np.uint8),8)
        if n<=1:continue
        k=1+int(np.argmax(stats[1:,cv2.CC_STAT_AREA]));mask=labels==k;area=int(mask.sum())
        if area<24:continue
        gray=cv2.cvtColor(rgb,cv2.COLOR_RGB2GRAY)
        texture=np.abs(gray-cv2.GaussianBlur(gray,(0,0),1.2))
        quality=float(np.mean(texture[mask]));confidence=float(np.mean(skin[mask]))
        score=area*confidence*(.25+.75*float(smooth(.003,.025,quality)))
        candidates.append((score,-i,mask,dict(index=i,area=area,confidence=confidence,texture=quality,score=score,amplitude=float(np.median(skin[mask])))))
    if not candidates:raise ValueError('No automatic confident skin anchor')
    best=max(candidates,key=lambda x:(x[0],x[1]));return best[3],best[2]


def warp_anchor(current,anchor,mask):
    validate_image(current);validate_image(anchor)
    if current.shape!=anchor.shape or mask.shape!=current.shape[:2] or not np.isfinite(mask).all() or mask.min()<0 or mask.max()>1:raise ValueError('Bad anchor geometry')
    h,w=mask.shape;gray=lambda a:np.rint(cv2.cvtColor(a,cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
    f=cv2.calcOpticalFlowFarneback(gray(current),gray(anchor),None,.5,4,21,5,7,1.5,0)
    b=cv2.calcOpticalFlowFarneback(gray(anchor),gray(current),None,.5,4,21,5,7,1.5,0)
    xy=np.stack(np.meshgrid(np.arange(w),np.arange(h)),-1).astype(np.float32)+f
    remap=lambda a:cv2.remap(a,xy[...,0],xy[...,1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
    valid=(xy[...,0]>=0)&(xy[...,0]<=w-1)&(xy[...,1]>=0)&(xy[...,1]<=h-1)
    fb=np.linalg.norm(f+remap(b),axis=-1);appearance=np.abs(current-remap(anchor)).mean(-1)
    trust=valid*(1-smooth(.5,2.,fb))*(1-smooth(.025,.12,appearance))
    return remap(mask.astype(np.float32)),trust.astype(np.float32)


def cut_between(images,a,b):
    # Conservative coarse cut veto: camera motion alone must not look like a
    # cut merely because pixel positions changed. Similar-palette cuts remain
    # a limitation; geometric checks are still required for flow propagation.
    for j in range(min(a,b),max(a,b)):
        left=cv2.resize(images[j],(32,32));right=cv2.resize(images[j+1],(32,32))
        if float(np.abs(left-right).mean())<=.15:continue
        hist=lambda x:np.histogramdd(x.reshape(-1,3),bins=(8,8,8),range=((0,1),)*3)[0]/1024
        if .5*float(np.abs(hist(left)-hist(right)).sum())>.65:return True
    return False


def combine_prediction(base,forward,backward,trust,person,amplitude):
    shape=base.shape[:2];validate_weights(base,shape)
    for m in (forward,backward,trust,person):
        if m.shape!=shape or not np.isfinite(m).all() or m.min()<0 or m.max()>1:raise ValueError('Invalid prediction')
    if not np.isfinite(amplitude) or not 0<=amplitude<=1:raise ValueError('Invalid anchor amplitude')
    agreed=np.minimum(forward,backward)
    confidence=(1-smooth(.15,.5,np.abs(forward-backward)))*trust*smooth(.1,.3,person)
    confidence*=1-smooth(.6,.95,base[...,2])
    weight=confidence*smooth(.1,.5,agreed)
    target=agreed*amplitude
    result=base.copy();result[...,0]=(1-weight)*base[...,0]+weight*target
    others=result[...,1:];remaining=np.maximum(0,1-result[...,0,None]);others*=np.minimum(1,remaining/np.maximum(others.sum(-1,keepdims=True),1e-12))
    result[weight==0]=base[weight==0]
    validate_weights(result,shape);return result,weight


def combine_geometry(sam_forward,sam_backward,flow_forward,flow_backward,flow_trust):
    """Flow may refine agreed geometry, never veto SAM when flow is unreliable."""
    shape=sam_forward.shape
    for m in (sam_forward,sam_backward,flow_forward,flow_backward,flow_trust):
        if m.shape!=shape or not np.isfinite(m).all() or m.min()<0 or m.max()>1:
            raise ValueError('Invalid hybrid geometry')
    compatible=(1-smooth(.1,.3,np.abs(flow_forward-flow_backward)))
    compatible*=1-smooth(.1,.3,np.abs(np.minimum(flow_forward,flow_backward)-np.minimum(sam_forward,sam_backward)))
    weight=.25*flow_trust*compatible*smooth(.1,.5,np.minimum(sam_forward,sam_backward))
    return ((1-weight)*sam_forward+weight*flow_forward,
            (1-weight)*sam_backward+weight*flow_backward,weight)
