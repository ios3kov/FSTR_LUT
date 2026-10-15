"""Research appearance protection: bounded color regions, not skin segmentation.

Same-colored hands/props inside a head region are intentionally not excluded.
No stored track history, identity recognition, or global warm-color selection.
"""
from dataclasses import replace
import cv2
import numpy as np
from person_head_analysis import same_person,stabilized_source_height,recover_coarse_regions
from smart_analysis import smoothstep,validate_rgb
from turn_skin_recovery import proposal_region


def region_box(person):
    return np.asarray(person.anchor.box if person.anchor is not None else person.head,float)


def soft_region(box,shape):
    region=proposal_region(box,shape)
    return (cv2.GaussianBlur(region,(0,0),max(.8,(box[3]-box[1])*.06))*region).astype(np.float32)


def detail_confidence(rgb,person):
    """Detail availability heuristic, not a calibrated physical blur estimate."""
    if person.anchor is None or not person.anchor.confirmed:return 0.
    b=region_box(person);x0,y0=np.floor(b[:2]).astype(int);x1,y1=np.ceil(b[2:]).astype(int)
    gray=cv2.cvtColor(cv2.resize(rgb[y0:y1,x0:x1],(64,64)),cv2.COLOR_RGB2GRAY)
    high=gray-cv2.GaussianBlur(gray,(0,0),1.2)
    energy=float(np.sqrt(np.mean(high[8:-8,8:-8]**2)))
    return float(smoothstep(.01,.04,energy))


def color_region_window(images,people,center,cuts):
    """3..5 explicit frames. Soft fallback exists at any face size.

    Details can modulate at most half the mask, avoiding a punctured region
    when a detailed face observation disappears. Final mask is <=0.65.
    """
    n=len(images)
    if not 3<=n<=5 or len(people)!=n or len(cuts)!=n-1 or not 0<=center<n:raise ValueError('Invalid window')
    shape=images[center].shape[:2]
    for rgb,entries in zip(images,people):
        validate_rgb(rgb)
        if rgb.dtype!=np.float32 or rgb.shape!=images[center].shape or min(shape)<32 or max(shape)>2048 or shape[0]*shape[1]>2097152 or len(entries)>20:raise ValueError('Invalid frame')
        for p in entries:
            geometry=np.asarray((*p.nose,*(p.shoulders or ())),float)
            if geometry.shape!=((2,) if p.shoulders is None else (6,)) or not np.isfinite(geometry).all():raise ValueError('Invalid association geometry')
            b=region_box(p)
            if b.shape!=(4,) or not np.isfinite(b).all() or b.min()<0 or b[2]>shape[1] or b[3]>shape[0] or np.any(b[2:]<=b[:2]) or not np.isfinite(p.source_height) or p.source_height<=0:raise ValueError('Invalid region')
            if p.anchor is not None:
                m=p.anchor.mask
                if m.shape!=shape or m.dtype!=np.float32 or not np.isfinite(m).all() or m.min()<0 or m.max()>1:raise ValueError('Invalid detail')
    result=np.zeros(shape,np.float32);records=[]
    for p in people[center]:
        neighbors=[]
        for direction in (-1,1):
            for step in (1,2):
                j=center+step*direction
                if not 0<=j<n or cuts[min(j,j-direction)]:break
                q=same_person(p,people[j])
                if q is None:
                    if people[j]:break
                    continue
                if same_person(q,people[center]) is not p:break
                neighbors.append((j,q));break
        height=stabilized_source_height(p,[q for _,q in neighbors]);b=region_box(p)
        if len(neighbors)==2:
            (lo,left),(hi,right)=neighbors;t=(center-lo)/(hi-lo)
            b=.75*b+.25*((1-t)*region_box(left)+t*region_box(right))
        region=soft_region(b,shape);confidence=detail_confidence(images[center],p)
        if neighbors:
            confidence=min(confidence,.75*confidence+.25*np.median([detail_confidence(images[j],q) for j,q in neighbors]))
        # No detailed observation means no borrowed facial holes on this frame.
        weight=.5*float(smoothstep(64,192,height))*confidence if p.anchor is not None and p.anchor.confirmed else 0.
        detailed=np.minimum(region,p.anchor.mask) if p.anchor is not None and p.anchor.confirmed else region
        mask=.65*float(smoothstep(24,32,height))*((1-weight)*region+weight*detailed)
        result=np.maximum(result,mask)
        records.append(dict(status='color_region',source_height=p.source_height,effective_height=height,box=b.tolist(),detail_weight=float(weight),tracked_neighbors=len(neighbors)))
    # Same two-sided gap contract, but appearance mode deliberately retains
    # similar-colored hands/props and allows large approximate objects too.
    recovered,trace=recover_coarse_regions(images,people,center,cuts,appearance=True)
    result=np.maximum(result,recovered);records.extend(trace)
    return result.astype(np.float32),records
