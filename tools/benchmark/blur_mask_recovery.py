"""Research-only, nonrecursive blur dropout recovery from two raw neighbors.

A photometric rejection is not semantic occluder recognition. Similar-colored
occluders remain ambiguous; this module must not be promoted on numeric tests.
"""
import cv2
import numpy as np
from smart_analysis import validate_rgb, connected_indices
from turn_skin_recovery import checked_warp


def sharpness(rgb, mask):
    gray=cv2.cvtColor(rgb.astype(np.float32),cv2.COLOR_RGB2GRAY)
    edge=cv2.Laplacian(gray,cv2.CV_32F)
    return float(np.sum(edge*edge*mask)/max(float(mask.sum()),1e-8))


def recover_blur(images, support, observed, center, cuts):
    """Explicit radius2 window, immediate two-sided original observations only.

    Bounds output by existing current face support. Require both neighbors to
    retain >=80% support, current retention <70%, and local sharpness <75% of
    both neighbors. Thresholds are research heuristics, not calibrated scores.
    """
    ids=connected_indices(center,len(images),cuts)
    if len(support)!=len(images) or len(observed)!=len(images):raise ValueError('Window mismatch')
    shape=images[center].shape[:2]
    for j in ids:
        validate_rgb(images[j])
        if images[j].dtype!=np.float32 or min(images[j].shape[:2])<8 or max(images[j].shape[:2])>2048:raise ValueError('Float32 RGB, sides8..2048 required')
        if images[j].shape!=images[center].shape:raise ValueError('Image shape mismatch')
        for m in (support[j],observed[j]):
            if m.dtype!=np.float32 or m.shape!=shape or not np.isfinite(m).all() or m.min()<0 or m.max()>1:raise ValueError('Invalid mask')
        if np.any(observed[j]>support[j]):raise ValueError('Observed mask outside support')
    base=observed[center].copy()
    def refuse(reason):return base,dict(status='unchanged',reason=reason)
    mass=float(support[center].sum())
    if mass<1 or float(base.sum())/mass>=.7:return refuse('no_major_dropout')
    lo,hi=center-1,center+1
    if lo not in ids or hi not in ids:return refuse('two_sided_neighbors_unavailable')
    for j in (lo,hi):
        if support[j].sum()<1 or observed[j].sum()/support[j].sum()<.8:return refuse('weak_neighbor')
    s=[sharpness(images[j],support[j]) for j in (lo,center,hi)]
    if min(s[0],s[2])<1e-6 or s[1]/min(s[0],s[2])>=.75:return refuse('no_relative_blur')
    left=checked_warp(images[center],images[lo],observed[lo])
    right=checked_warp(images[center],images[hi],observed[hi])
    overlap=np.minimum(left,right)
    agreement=float(overlap.sum()/max(float(np.maximum(left,right).sum()),1e-8))
    if agreement<.45:return refuse('motion_disagreement')
    recovery=np.minimum(overlap,support[center])
    # Direction comes from actual local flow, not a global camera vector.
    gray=lambda x:np.rint(cv2.cvtColor(x.astype(np.float32),cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
    flow=cv2.calcOpticalFlowFarneback(gray(images[center]),gray(images[hi]),None,.5,3,15,3,5,1.2,0)
    region=recovery>.05
    if not region.any():return refuse('no_valid_correspondence')
    vector=np.median(flow[region],axis=0);speed=float(np.linalg.norm(vector))
    # A capped feather radius is an engineering heuristic, NOT exposure time
    # or recovered physical blur length. Inward only: cannot expand footprint.
    radius=float(min(2.,speed*.5)*(1-s[1]/min(s[0],s[2])))
    if speed>.1 and radius>.1:
        yy,xx=np.mgrid[:shape[0],:shape[1]].astype(np.float32);direction=vector/speed
        softened=np.zeros_like(recovery)
        for t in (-1.,-.5,0.,.5,1.):
            softened+=cv2.remap(recovery,xx+t*radius*direction[0],yy+t*radius*direction[1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)/5
        recovery=np.minimum(recovery,softened)
    result=np.maximum(base,recovery).astype(np.float32)
    return result,dict(status='recovered',neighbors=[lo,hi],sharpness=s,agreement=agreement,motion_vector=vector.tolist(),feather_radius=radius,before_mass=float(base.sum()),after_mass=float(result.sum()))
