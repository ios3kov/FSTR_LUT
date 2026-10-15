"""Research fusion of independent people evidence with scene probabilities.

All observations belong to the current frame. No stored tracks, guessed whole-body
skin, or manually selected boxes. Source scale is explicit, never preview height.
"""
import cv2
import numpy as np
from scene_analysis import smooth, validate_image, validate_weights


def detail_mix(source_height, quality, kind='face'):
    if not np.isfinite(source_height) or source_height<=0 or not np.isfinite(quality) or not 0<=quality<=1:
        raise ValueError('Invalid source size/detail confidence')
    if kind=='face': activation=(24,32);medium=(48,80);fine=(128,192)
    elif kind=='person': activation=(64,128);medium=(256,384);fine=(640,896)
    else: raise ValueError('Unknown region kind')
    m=float(smooth(*medium,source_height))*quality;f=float(smooth(*fine,source_height))*quality
    return float(smooth(*activation,source_height)),np.array([1-m,m*(1-f),m*f],np.float32)


def face_field(rgb, mask, records, source_scale_y):
    validate_image(rgb);h,w=rgb.shape[:2]
    if not np.isfinite(source_scale_y) or source_scale_y<=0:raise ValueError('Invalid source scale')
    if mask.shape!=(h,w) or not np.isfinite(mask).all() or mask.min()<0 or mask.max()>1:raise ValueError('Invalid face mask')
    result=np.zeros((h,w),np.float32);details=[];yy,xx=np.mgrid[:h,:w]
    for r in records:
        b=np.asarray(r['box'],float);score=float(r['detector_score'])
        if b.shape!=(4,) or not np.isfinite(b).all() or b.min()<0 or b[2]>w or b[3]>h or np.any(b[2:]<=b[:2]) or not np.isfinite(score) or not 0<=score<=1:raise ValueError('Invalid face observation')
        confirmed=r['status']=='confirmed' or (r['status'] in ('small_region','no_mesh') and r.get('scale_agreement',0)>=2)
        if not confirmed or score<.8:continue
        bw,bh=b[2:]-b[:2];cx,cy=(b[:2]+b[2:])/2
        radius=np.sqrt(((xx-cx)/(.48*bw))**2+((yy-cy)/(.5*bh))**2)
        region=(1-smooth(.55,1.,radius)).astype(np.float32)
        x0,y0=np.floor(b[:2]).astype(int);x1,y1=np.ceil(b[2:]).astype(int)
        gray=cv2.cvtColor(cv2.resize(rgb[y0:y1,x0:x1],(64,64)),cv2.COLOR_RGB2GRAY)
        high=gray-cv2.GaussianBlur(gray,(0,0),1.2)
        quality=float(smooth(.01,.04,float(np.sqrt(np.mean(high[8:-8,8:-8]**2)))))
        # Unconfirmed mesh never supplies detailed skin, even when another face
        # has a valid mask in the same frame. A broad head spot stays weak.
        if r['status']!='confirmed':quality=0.
        quality*=float(smooth(.05,.35,float(r.get('semantic_mean_inside',0.))))
        source_height=bh*source_scale_y
        mesh_entry=float(smooth(64,80,source_height))
        quality*=mesh_entry
        confirmation=1. if r.get('scale_agreement',0)>=2 else mesh_entry
        activation,mix=detail_mix(source_height,quality)
        detailed=mask*region;medium=cv2.GaussianBlur(detailed,(0,0),max(.6,bh*.025))*region
        field=activation*confirmation*score*(mix[0]*.35*region+mix[1]*medium+mix[2]*detailed)
        result=np.maximum(result,field)
        details.append(dict(box=b.tolist(),source_height=bh*source_scale_y,detail_quality=quality,mix=mix.tolist()))
    return result.astype(np.float32),details


def person_field(rgb, parts, people, source_scale_y):
    validate_image(rgb)
    if parts.ndim!=3 or parts.shape[2]!=6 or not np.isfinite(parts).all() or parts.min()<0 or parts.max()>1 or np.max(parts.sum(-1))>1.001:raise ValueError('Invalid person parts')
    if not np.isfinite(source_scale_y) or source_scale_y<=0:raise ValueError('Invalid source scale')
    h,w=parts.shape[:2]
    if rgb.shape[:2]!=(h,w):raise ValueError('Person input geometry mismatch')
    result=np.zeros((h,w,3),np.float32);details=[]
    material=np.stack([parts[...,2]+parts[...,3],parts[...,1],parts[...,4]],-1)
    for person in people:
        if person.shape!=(h,w) or not np.isfinite(person).all() or person.min()<0 or person.max()>1:raise ValueError('Invalid person evidence')
        support=smooth(.05,.25,person).astype(np.float64)
        mass=float(support.sum())
        if mass==0:continue
        # Moment extent varies continuously with probability. A .5 hard box
        # would switch an entire person on/off at a single confidence threshold.
        rowmass=support.sum(1);yy=np.arange(h,dtype=np.float64)
        center=float(rowmass@yy/mass)
        height=float(np.sqrt(12*float(rowmass@((yy-center)**2)/mass)+1))
        gray=cv2.cvtColor(rgb,cv2.COLOR_RGB2GRAY)
        high=gray-cv2.GaussianBlur(gray,(0,0),1.2)
        quality=float(smooth(.01,.04,float(np.sqrt(np.sum(high**2*support)/mass))))
        activation,mix=detail_mix(height*source_scale_y,quality,'person')
        activation*=float(smooth(.05,.25,float(person.max())))
        detailed=material*person[...,None]
        medium=cv2.GaussianBlur(detailed,(0,0),max(.6,height*.015))
        coarse=cv2.GaussianBlur(detailed,(0,0),max(.8,height*.045))*.5
        # Unknown parts remain unknown; a body mask is never all skin.
        field=activation*(mix[0]*coarse+mix[1]*medium+mix[2]*detailed)*person[...,None]
        result=np.maximum(result,field)
        details.append(dict(source_height=height*source_scale_y,mix=mix.tolist()))
    result/=np.maximum(1,result.sum(-1,keepdims=True))
    return result.astype(np.float32),details


def fuse_people(scene, people, face):
    """Spend existing probability mass; never add masks past one or recolor afar."""
    shape=scene.shape[:2];validate_weights(scene,shape)
    if people.shape!=(*shape,3) or not np.isfinite(people).all() or people.min()<0 or people.max()>1 or np.max(people.sum(-1))>1.000001:raise ValueError('Invalid people role mass')
    if face.shape!=shape or not np.isfinite(face).all() or face.min()<0 or face.max()>1:raise ValueError('Invalid face evidence')
    if not people.any() and not face.any():return scene.copy()
    roles=np.maximum(scene[...,:3],people)
    roles[...,0]=np.maximum(roles[...,0],face)
    roles/=np.maximum(1,roles.sum(-1,keepdims=True))
    remaining=np.maximum(0,1-roles.sum(-1,keepdims=True))
    other=scene[...,3:];factor=np.minimum(1,remaining/np.maximum(other.sum(-1,keepdims=True),1e-12))
    result=np.concatenate([roles,other*factor],-1).astype(np.float32)
    validate_weights(result,shape)
    return result
