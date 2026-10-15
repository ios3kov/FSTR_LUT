"""Bounded research face support and nonrecursive one/two-frame gap recovery.

No host API or video-mode hidden state. Profile fallback never seeds recovery.
Confidence gates are heuristics, not a guarantee of hand/occlusion recognition.
"""
import cv2
import numpy as np
from face_skin_analysis import FaceSkinSegmenter,crop_box,stabilize_face_mask
from smart_analysis import validate_rgb,smoothstep,connected_indices,stabilize_mask


def proposal_region(box,shape):
    """Inward feathered inner ellipse, only a bound on semantic evidence."""
    b=np.asarray(box,float)
    if b.shape!=(4,) or not np.isfinite(b).all():raise ValueError('Invalid proposal box')
    h,w=shape;size=b[2:]-b[:2]
    if min(size)<8 or b.min()<0 or b[2]>w or b[3]>h:return np.zeros(shape,np.float32)
    yy,xx=np.mgrid[:h,:w];cx,cy=(b[:2]+b[2:])/2
    distance=np.sqrt(((xx-cx)/(.47*size[0]))**2+((yy-cy)/(.47*size[1]))**2)
    return (1-smoothstep(.8,1.,distance)).astype(np.float32)


class TurnSkinSegmenter(FaceSkinSegmenter):
    def __init__(self,*args):
        super().__init__(*args,proposal_threshold=.6,minimum_semantic_mean=.25)

    def analyze_turn(self,rgb,minimum_mesh_height=0.):
        mesh,support,detail=self.analyze_with_support(rgb,minimum_mesh_height)
        h,w=rgb.shape[:2];proposal=np.zeros((h,w),np.float32);semantic=proposal.copy();profile=proposal.copy()
        viable=[]
        for face in detail['faces']:
            box=np.array(face['box']);region=proposal_region(box,(h,w))
            if not region.any():continue
            if face['status']=='small_region':
                # Approximate head-color region: no mesh or skin inference.
                proposal=np.maximum(proposal,region)
                semantic=np.maximum(semantic,region)
                face['coarse_region_only']=True
                continue
            x0,y0,x1,y1=crop_box(box,(h,w),2.6)
            maps=self.skin._infer(rgb[y0:y1,x0:x1])
            # Use face class alone. Body-skin would directly accept hands.
            face_weight=smoothstep(.65,.9,maps[...,3]).astype(np.float32)
            local=region[y0:y1,x0:x1]*face_weight
            evidence=float(local.sum()/max(1,(region>0).sum()))
            face['current_face_semantic_mean']=evidence
            if evidence<.2:continue
            viable.append(face)
            proposal=np.maximum(proposal,region)
            semantic[y0:y1,x0:x1]=np.maximum(semantic[y0:y1,x0:x1],local)
            if (face['status']=='no_mesh' and face['detector_score']>=.8
                    and face['scale_agreement']>=2 and evidence>=.5):
                # No mesh feature holes in this branch: explicitly experimental.
                profile[y0:y1,x0:x1]=np.maximum(profile[y0:y1,x0:x1],local)
                face['profile_fallback']=True
        detail['viable_proposals']=len(viable)
        detail['mesh_faces']=sum(f['status']=='confirmed' for f in detail['faces'])
        return dict(mesh=mesh,support=support,proposal=proposal,semantic=semantic,profile=profile,detail=detail)


def checked_warp(current,other,mask):
    """Direct target-to-source flow; checked both ways, bounded and photometric."""
    validate_rgb(current);validate_rgb(other)
    if current.shape!=other.shape or mask.shape!=current.shape[:2]:raise ValueError('Warp shape mismatch')
    if not np.isfinite(mask).all() or mask.min()<0 or mask.max()>1:raise ValueError('Invalid source mask')
    gray=lambda a:np.rint(cv2.cvtColor(a,cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
    a,b=gray(current),gray(other)
    forward=cv2.calcOpticalFlowFarneback(a,b,None,.5,3,15,3,5,1.2,0)
    backward=cv2.calcOpticalFlowFarneback(b,a,None,.5,3,15,3,5,1.2,0)
    h,w=mask.shape;yy,xx=np.mgrid[:h,:w].astype(np.float32);mx=xx+forward[...,0];my=yy+forward[...,1]
    remap=lambda x:cv2.remap(x,mx,my,cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
    residual=np.linalg.norm(forward+remap(backward),axis=-1)
    photo=np.abs(current-remap(other)).mean(-1)
    # Texture is correspondence evidence, not skin classification. Uniform
    # fields can otherwise yield a deceptively perfect zero-flow consistency.
    lum=cv2.cvtColor(current,cv2.COLOR_RGB2GRAY)
    variance=np.maximum(cv2.blur(lum*lum,(7,7))-cv2.blur(lum,(7,7))**2,0)
    valid=(mx>=1)&(mx<w-2)&(my>=1)&(my<h-2)&(residual<1)&(photo<.08)&(variance>.00002)
    confidence=valid*(1-smoothstep(.5,1.,residual))*(1-smoothstep(.04,.08,photo))
    return (remap(mask)*confidence).astype(np.float32)


def recover_short_gap(images,mesh_masks,current_evidence,mesh_counts,proposal_counts,center,cuts):
    """Recover at most two missing frames from original mesh-confirmed seeds.

    Caller supplies raw mesh masks only; recovered/profile arrays are not seeds.
    Returns an additional mask and auditable provenance, never a cached history.
    """
    indices=connected_indices(center,len(images),cuts)
    n=len(images)
    if any(len(x)!=n for x in (mesh_masks,current_evidence,mesh_counts,proposal_counts)):
        raise ValueError('Recovery window mismatch')
    shape=images[center].shape[:2]
    for j in indices:
        validate_rgb(images[j])
        if images[j].shape!=images[center].shape:raise ValueError('Recovery image shape mismatch')
        for arr in (mesh_masks[j],current_evidence[j]):
            if arr.shape!=shape or not np.isfinite(arr).all() or arr.min()<0 or arr.max()>1:
                raise ValueError('Invalid recovery mask')
        if not isinstance(mesh_counts[j],(int,np.integer)) or mesh_counts[j]<0 or not isinstance(proposal_counts[j],(int,np.integer)) or proposal_counts[j]<0:
            raise ValueError('Invalid face counts')
    zero=np.zeros(shape,np.float32)
    def refuse(reason):return zero,dict(status='abstained',reason=reason)
    if mesh_counts[center]!=0 or mesh_masks[center].any():return refuse('current_mesh_present')
    if proposal_counts[center]!=1 or not current_evidence[center].any():return refuse('current_evidence_absent_or_ambiguous')
    seeds=[j for j in indices if mesh_counts[j]==1 and mesh_masks[j].any()]
    left=[j for j in seeds if j<center];right=[j for j in seeds if j>center]
    if not left or not right:return refuse('two_sided_seeds_unavailable')
    lo,hi=max(left),min(right)
    if hi-lo>3:return refuse('gap_longer_than_two')
    if any(mesh_counts[j]>1 for j in range(lo,hi+1)):return refuse('ambiguous_faces')
    a=checked_warp(images[center],images[lo],mesh_masks[lo])
    b=checked_warp(images[center],images[hi],mesh_masks[hi])
    # Both warped seed footprints must agree with this one current proposal.
    target=current_evidence[center]>0
    for warped in (a,b):
        total=float(warped.sum())
        if total<=0 or float(warped[target].sum())/total<.6:return refuse('proposal_motion_disagreement')
    overlap=np.minimum(a,b);union=np.maximum(a,b)
    if float(overlap.sum())/max(1e-8,float(union.sum()))<.45:return refuse('two_sided_mask_disagreement')
    recovered=np.minimum(overlap,current_evidence[center]).astype(np.float32)
    if not np.any(recovered>.05):return refuse('motion_or_semantics_rejected')
    return recovered,dict(status='temporal_recovery',endpoints=[lo,hi],pixels=int((recovered>0).sum()))


def combine_turn_window(images,mesh,support,profile,proposal,semantic,counts,proposals,center,cuts):
    """Whole mask pipeline uses raw observations only, never recovered neighbors."""
    indices=connected_indices(center,len(images),cuts)
    if any(len(x)!=len(images) for x in (support,profile,proposal)):
        raise ValueError('Combination window mismatch')
    # Only access this window. List entries outside it are intentionally unused.
    spatial=list(mesh)
    for j in indices:
        for a in (support[j],profile[j],proposal[j]):
            if a.shape!=images[center].shape[:2] or not np.isfinite(a).all() or a.min()<0 or a.max()>1:
                raise ValueError('Invalid combination support')
        spatial[j]=np.maximum(mesh[j],profile[j])
    base=stabilize_face_mask(images,mesh,support,center,cuts)
    profiled=np.minimum(stabilize_mask(images,spatial,center,cuts),np.maximum(support[center],proposal[center]))
    recovered,info=recover_short_gap(images,mesh,semantic,counts,proposals,center,cuts)
    return dict(mesh=base,spatial=profiled,recovered=np.maximum(base,recovered),smart=np.maximum(profiled,recovered),recovery_only=recovered),info
