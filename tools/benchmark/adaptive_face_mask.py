"""Research-only size-adaptive face masks and bounded, order-independent tracking.

Coordinates are analysis pixels; source_scale_y maps heights to source pixels.
No identity recognition, host state, inferred occlusion or recursive recovery.
"""
from dataclasses import dataclass
import numpy as np
from face_skin_analysis import box_iou
from blur_mask_recovery import structural_warp as checked_warp
from smart_analysis import smoothstep, validate_rgb


@dataclass(frozen=True)
class FaceObservation:
    box: tuple
    confidence: float
    confirmed: bool
    medium: np.ndarray
    detailed: np.ndarray
    detail_confidence: float = 1.
    simple_allowed: bool = False


def _validate(obs, shape):
    b=np.asarray(obs.box,dtype=float)
    h,w=shape
    if (b.shape!=(4,) or not np.isfinite(b).all() or b.min()<0
            or b[2]>w or b[3]>h or np.any(b[2:]<=b[:2])):
        raise ValueError('Invalid face box')
    for c in (obs.confidence,obs.detail_confidence):
        if not np.isfinite(c) or not 0<=c<=1:raise ValueError('Invalid confidence')
    for mask in (obs.medium,obs.detailed):
        if mask.shape!=shape or not np.isfinite(mask).all() or mask.min()<0 or mask.max()>1:
            raise ValueError('Invalid face mask')
    return b


def spatial_mask(obs, shape, source_scale_y=1.):
    """24/64/160px nominal bands; continuous24..32,48..80,128..192 ramps.

    Simple ellipse has maximum35% protection. Detailed masks are supplied by
    the caller; missing/uncertain detail must not be fabricated from landmarks.
    """
    if not np.isfinite(source_scale_y) or source_scale_y<=0:raise ValueError('Invalid source scale')
    b=_validate(obs,shape);height=(b[3]-b[1])*source_scale_y
    empty=np.zeros(shape,np.float32)
    if not obs.confirmed or obs.confidence<.8:return empty
    y,x=np.mgrid[:shape[0],:shape[1]];cx,cy=(b[:2]+b[2:])/2;bw,bh=b[2:]-b[:2]
    radius=np.sqrt(((x-cx)/(.38*bw))**2+((y-cy)/(.43*bh))**2)
    simple=(1-smoothstep(.65,1.,radius))*.35*bool(obs.simple_allowed)
    medium_weight=float(smoothstep(48,80,height))*obs.detail_confidence
    detailed_weight=float(smoothstep(128,192,height))*obs.detail_confidence
    detail=(1-detailed_weight)*obs.medium+detailed_weight*obs.detailed
    result=((1-medium_weight)*simple+medium_weight*detail)
    return (result*float(smoothstep(24,32,height))*obs.confidence).astype(np.float32)


def _unique_match(box, observations):
    # Reject competing overlapping candidates; nearest alone is unsafe at crossings.
    candidates=[o for o in observations if box_iou(np.asarray(box),np.asarray(o.box))>.3]
    return candidates[0] if len(candidates)==1 and candidates[0].confirmed and candidates[0].confidence>=.8 else None


def track_window(images, observations, center, cuts, source_scale_y=1., gap_evidence=None):
    """At most5explicit frames, center +/−2; cuts[i] separates i−1/i.

    Current faces remain anchors. Neighbor agreement smooths masks via validated
    bidirectional optical flow. One/two missing frames require unique two-sided
    detections, agreeing motion and caller-supplied current semantic evidence.
    Without current evidence, gap recovery is disabled; it is not an occluder
    classifier. Recovery never becomes a future seed.
    """
    n=len(images)
    if not 1<=n<=5 or len(observations)!=n or len(cuts)!=n or not 0<=center<n:
        raise ValueError('Invalid bounded window')
    shape=images[center].shape[:2]
    for rgb,faces in zip(images,observations):
        validate_rgb(rgb)
        if (rgb.dtype!=np.float32 or rgb.shape!=images[center].shape or len(faces)>16
                or min(shape)<8 or max(shape)>2048):raise ValueError('Invalid window geometry')
        for face in faces:_validate(face,shape)
    if not np.isfinite(source_scale_y) or source_scale_y<=0:raise ValueError('Invalid source scale')
    if gap_evidence is not None:
        if (gap_evidence.shape!=shape or not np.isfinite(gap_evidence).all()
                or gap_evidence.min()<0 or gap_evidence.max()>1):
            raise ValueError('Invalid current semantic evidence')
    allowed=[center]
    for direction in (-1,1):
        for step in (1,2):
            j=center+direction*step
            if j<0 or j>=n or cuts[max(j,j-direction)]:break
            allowed.append(j)
    result=np.zeros(shape,np.float32);records=[]
    current=[o for o in observations[center] if o.confirmed and o.confidence>=.8]
    for face in current:
        mask=spatial_mask(face,shape,source_scale_y);warps=[]
        for j in allowed:
            if j==center:continue
            other=_unique_match(face.box,observations[j])
            if other is None or _unique_match(other.box,observations[center]) is not face:continue
            warped=checked_warp(images[center],images[j],spatial_mask(other,shape,source_scale_y))
            confidence=checked_warp(images[center],images[j],np.ones(shape,np.float32))
            valid=confidence>.1
            normalized=np.divide(warped,confidence,out=mask.copy(),where=valid)
            # Invalid correspondence is unknown, never evidence for removing skin.
            warps.append(np.where(valid,np.minimum(normalized,mask),mask))
        if warps:mask=.75*mask+.25*np.mean(warps,axis=0)
        result=np.maximum(result,mask);records.append(dict(status='current',neighbors=len(warps)))
    # Conservative gap route: only one face on each endpoint, no other current
    # proposals (even unconfirmed) that might signal identity ambiguity.
    if not current and not observations[center] and gap_evidence is not None:
        left=[j for j in allowed if j<center and len(observations[j])==1]
        right=[j for j in allowed if j>center and len(observations[j])==1]
        if left and right:
            lo,hi=max(left),min(right);a,b=observations[lo][0],observations[hi][0]
            interior_clear=all(not observations[j] for j in range(lo+1,hi))
            if hi-lo<=3 and interior_clear and _unique_match(a.box,[b]) is b and _unique_match(b.box,[a]) is a:
                wa=checked_warp(images[center],images[lo],spatial_mask(a,shape,source_scale_y))
                wb=checked_warp(images[center],images[hi],spatial_mask(b,shape,source_scale_y))
                overlap=np.minimum(wa,wb);union=np.maximum(wa,wb)
                if overlap.sum()>0 and overlap.sum()/max(float(union.sum()),1e-8)>=.6:
                    result=.5*np.minimum(overlap,gap_evidence)
                    if result.any():records.append(dict(status='two_sided_gap',endpoints=[lo,hi]))
    return result.astype(np.float32),records
