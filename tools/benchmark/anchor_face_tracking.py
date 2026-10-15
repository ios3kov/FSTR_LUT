"""Research anchor-to-current face transfer; explicit window, no recovered seeds.

Local feature motion estimates geometry independently of current face detection.
Photometric/structural checks and current evidence constrain transferred pixels.
Neither flow nor semantic confidence proves absence of a skin-colored occluder.
"""
from dataclasses import dataclass
import cv2
import numpy as np
from smart_analysis import validate_rgb, smoothstep
from blur_mask_recovery import sharpness, structural_warp


@dataclass(frozen=True)
class FaceAnchor:
    box: tuple
    mask: np.ndarray
    confirmed: bool = True
    exclusion: np.ndarray | None = None


def _mask(mask,shape):
    if (mask.shape!=shape or mask.dtype!=np.float32 or not np.isfinite(mask).all()
            or mask.min()<0 or mask.max()>1):raise ValueError('Invalid mask')


def _gray(rgb):return np.rint(cv2.cvtColor(rgb,cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)


def _fit(points,target):
    x,y=points.T;matrix=np.zeros((2*len(points),4),float)
    matrix[0::2]=np.c_[x,-y,np.ones(len(x)),np.zeros(len(x))]
    matrix[1::2]=np.c_[y,x,np.zeros(len(x)),np.ones(len(x))]
    a,b,tx,ty=np.linalg.lstsq(matrix,target.reshape(-1),rcond=None)[0]
    return np.array([[a,-b,tx],[b,a,ty]],np.float32)


def track_anchor(source,current,anchor):
    """Deterministic similarity fit from mutually checked local feature tracks."""
    h,w=source.shape[:2];box=np.asarray(anchor.box,float)
    region=np.zeros((h,w),np.uint8);x0,y0=np.floor(box[:2]).astype(int);x1,y1=np.ceil(box[2:]).astype(int)
    region[y0:y1,x0:x1]=255
    a,b=_gray(source),_gray(current)
    points=cv2.goodFeaturesToTrack(a,100,.01,2,mask=region,blockSize=3)
    if points is None or len(points)<6:return None,dict(reason='few_anchor_features')
    target,status,error=cv2.calcOpticalFlowPyrLK(a,b,points,None,winSize=(21,21),maxLevel=3)
    if target is None:return None,dict(reason='forward_flow_failed')
    backward,back_status,_=cv2.calcOpticalFlowPyrLK(b,a,target,None,winSize=(21,21),maxLevel=3)
    if backward is None:return None,dict(reason='backward_flow_failed')
    p=points[:,0];q=target[:,0]
    valid=(status[:,0]>0)&(back_status[:,0]>0)&(error[:,0]<30)&(np.linalg.norm(backward[:,0]-p,axis=1)<1.5)
    valid &= np.isfinite(q).all(1)&(q[:,0]>=0)&(q[:,0]<w)&(q[:,1]>=0)&(q[:,1]<h)
    p,q=p[valid],q[valid]
    if len(p)<6:return None,dict(reason='unreliable_tracks')
    for _ in range(3):
        matrix=_fit(p,q);pred=p@matrix[:,:2].T+matrix[:,2];keep=np.linalg.norm(pred-q,axis=1)<=1.5
        if keep.all():break
        p,q=p[keep],q[keep]
        if len(p)<6:return None,dict(reason='inconsistent_motion')
    matrix=_fit(p,q)
    if np.any(np.linalg.norm(p@matrix[:,:2].T+matrix[:,2]-q,axis=1)>1.5):
        return None,dict(reason='final_fit_residual')
    scale=float(np.hypot(matrix[0,0],matrix[1,0]));angle=float(np.degrees(np.arctan2(matrix[1,0],matrix[0,0])))
    center=(box[:2]+box[2:])/2;motion=matrix[:,:2]@center+matrix[:,2]-center
    extent=np.ptp(p,axis=0)/(box[2:]-box[:2])
    if not .8<=scale<=1.25 or abs(angle)>30 or np.linalg.norm(motion)>.5*(box[3]-box[1]) or min(extent)<.25:
        return None,dict(reason='unreliable_geometry')
    return matrix,dict(points=len(p),scale=scale,angle=angle,motion=motion.tolist())


def coarse_weight(ratio):
    """Continuous weak-mask preference for strong blur OR uncertain detail."""
    blur=1-float(smoothstep(.15,.75,ratio))
    uncertainty=float(smoothstep(.55,.85,ratio))
    return uncertainty+(1-uncertainty)*blur


def transfer_blurred_face(images,anchors,center,cuts,current_evidence,occlusion):
    """3..5frames; n−1cuts. Nearest single confirmed anchors on both sides.

    Current detector/mesh may be absent. Explicit current face evidence and
    occlusion exclusion are still required. Both masks are caller snapshots;
    zeros in occlusion mean no supplied veto, NOT verified no occlusion.
    At most2interior frames, no competing anchor in the interior.
    """
    n=len(images)
    if not 3<=n<=5 or len(anchors)!=n or len(cuts)!=n-1 or not 0<=center<n:raise ValueError('Invalid anchor window')
    shape=images[center].shape[:2];h,w=shape
    for rgb,items in zip(images,anchors):
        validate_rgb(rgb)
        if rgb.dtype!=np.float32 or rgb.shape!=images[center].shape or min(shape)<32 or max(shape)>2048:raise ValueError('Invalid frame')
        for item in items:
            b=np.asarray(item.box,float)
            if b.shape!=(4,) or not np.isfinite(b).all() or b.min()<0 or b[2]>w or b[3]>h or np.any(b[2:]<=b[:2]):raise ValueError('Invalid anchor box')
            _mask(item.mask,shape)
            if item.exclusion is not None:_mask(item.exclusion,shape)
    _mask(current_evidence,shape);_mask(occlusion,shape)
    zero=np.zeros(shape,np.float32)
    def refuse(reason,**detail):return zero,dict(status='abstained',reason=reason,**detail)
    if anchors[center]:return refuse('current_anchor_present')
    left=[j for j in range(max(0,center-2),center) if anchors[j]]
    right=[j for j in range(center+1,min(n,center+3)) if anchors[j]]
    if not left or not right:return refuse('missing_two_sided_anchors')
    lo,hi=max(left),min(right)
    if hi-lo>3 or any(cuts[lo:hi]) or len(anchors[lo])!=1 or len(anchors[hi])!=1:return refuse('cut_long_gap_or_ambiguity')
    if any(anchors[j] for j in range(lo+1,hi)):return refuse('interior_anchor')
    warped=[];simple=[];guards=[];details=[];scores=[];exclusions=[]
    for j in (lo,hi):
        anchor=anchors[j][0]
        if not anchor.confirmed or anchor.mask.sum()<1:return refuse('weak_anchor')
        matrix,detail=track_anchor(images[j],images[center],anchor);details.append(detail)
        if matrix is None:return refuse('tracking_failed',tracking=details)
        warp=lambda m:cv2.warpAffine(m,matrix,(w,h),flags=cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
        observed=warp(anchor.mask);warped.append(observed)
        b=np.asarray(anchor.box);cx,cy=(b[:2]+b[2:])/2;bw,bh=b[2:]-b[:2];yy,xx=np.mgrid[:h,:w]
        radius=np.sqrt(((xx-cx)/(.38*bw))**2+((yy-cy)/(.43*bh))**2)
        coarse=(1-smoothstep(.65,1.,radius)).astype(np.float32);simple.append(warp(coarse))
        # Geometry comes from anchor tracking; structural evidence independently
        # vetoes texture changes. Its failures remain unknown, never filled in.
        dense=cv2.calcOpticalFlowFarneback(_gray(images[center]),_gray(images[j]),None,.5,3,15,3,5,1.2,0)
        inverse=cv2.invertAffineTransform(matrix)
        predicted_x=inverse[0,0]*xx+inverse[0,1]*yy+inverse[0,2]
        predicted_y=inverse[1,0]*xx+inverse[1,1]*yy+inverse[1,2]
        disagreement=np.hypot(xx+dense[...,0]-predicted_x,yy+dense[...,1]-predicted_y)
        geometric_gate=(1-smoothstep(1.,2.,disagreement)).astype(np.float32)
        guards.append(structural_warp(images[center],images[j],np.ones(shape,np.float32))*geometric_gate)
        exclusions.append(warp(anchor.exclusion) if anchor.exclusion is not None else zero)
        # Match geometry before comparing detail energy; still a blur heuristic.
        scores.append((sharpness(warp(images[j]),observed),sharpness(images[center],observed)))
    common=np.minimum(*warped);union=np.maximum(*warped)
    agreement=float(common.sum()/max(float(union.sum()),1e-8))
    if agreement<.6:return refuse('anchor_disagreement',agreement=agreement)
    ratios=[cur/max(src,1e-8) for src,cur in scores]
    if min(src for src,_ in scores)<1e-6:return refuse('weak_anchor_detail',ratios=ratios)
    # Detail energy is affected by resampling and illumination, so uncertainty
    # cannot veto independently tracked geometry. Unknown detail uses a weak
    # coarse mask; only relative-blur evidence permits a detailed contribution.
    uncertain=max(ratios)>=.75
    blur=coarse_weight(max(ratios))
    # Move gradually toward weak coarse support; do not paint outside the
    # two transported facial envelopes. Explicit foreground veto is last.
    coarse=np.minimum(*simple)*.35
    candidate=(1-blur)*common+blur*coarse
    gate=np.minimum(*guards)*current_evidence*(1-occlusion)*(1-np.maximum(*exclusions))
    result=(candidate*gate).astype(np.float32)
    if result.sum()<1:return refuse('insufficient_current_evidence',tracking=details)
    return result,dict(status='transferred',endpoints=[lo,hi],agreement=agreement,blur_weight=blur,detail_uncertain=uncertain,sharpness_ratios=ratios,tracking=details,mass=float(result.sum()))
