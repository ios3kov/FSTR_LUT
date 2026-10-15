"""Research-only automatic person/head evidence, face anchors and hand vetoes.

Independent IMAGE calls, no user-selected face, no persistent identity tracker.
Pose head boxes and hand hulls are geometry heuristics, not pixel segmentation.
"""
from dataclasses import dataclass,replace
from pathlib import Path
import hashlib
import cv2
import numpy as np
from smart_analysis import validate_rgb,smoothstep
from face_skin_analysis import crop_box,box_iou,MAX_FACES
from turn_skin_recovery import TurnSkinSegmenter,proposal_region
from anchor_face_tracking import FaceAnchor,transfer_blurred_face
from blur_mask_recovery import structural_warp

POSE_SHA='4eaa5eb7a98365221087693fcc286334cf0858e2eb6e15b506aa4a7ecdcec4ad'
HAND_SHA='fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1'


@dataclass(frozen=True)
class PersonEvidence:
    head: tuple
    nose: tuple
    shoulders: tuple | None
    person: np.ndarray
    skin: np.ndarray
    hands: np.ndarray
    anchor: FaceAnchor | None
    source_height: float = 192.
    light_region: bool = False
    approximate_only: bool = False


def size_adaptive_region(person,detailed):
    """Source-pixel bands. Tiny areas use soft photometric evidence, no mesh.

    Only attenuates the detailed branch above64px. Small-region support must
    already be automatically confirmed; brightness alone never creates it.
    """
    height=person.source_height
    if not np.isfinite(height) or height<=0:raise ValueError('Invalid source face height')
    box=np.asarray(person.anchor.box if person.anchor is not None else person.head);sigma=max(.8,(box[3]-box[1])*.1)
    region=proposal_region(box,detailed.shape)
    spot=cv2.GaussianBlur(region,(0,0),sigma)*region*.35
    activation=float(smoothstep(24,32,height));medium=float(smoothstep(64,80,height))
    detail=float(smoothstep(128,192,height))
    # Blurring then capping cannot paint known excluded holes back in.
    simple_detail=np.minimum(detailed,cv2.GaussianBlur(detailed,(0,0),sigma*.5))
    result=activation*((1-medium)*spot+medium*((1-detail)*simple_detail+detail*detailed))
    return np.minimum(result,1-person.hands).astype(np.float32)


def head_geometry(points,shape):
    """33x4(x,y,visibility,presence), pixel positions; conservative head proxy."""
    if points.shape!=(33,4) or not np.isfinite(points).all():raise ValueError('Invalid pose landmarks')
    chosen=points[[0,2,5,7,8,11,12]]
    if np.any(chosen[:,2:]<.5) or np.any(chosen[:,2:]>1):return None
    nose=points[0,:2];ears=points[[7,8],:2];shoulders=points[[11,12],:2]
    earspan=float(np.linalg.norm(ears[0]-ears[1]));span=float(np.linalg.norm(shoulders[0]-shoulders[1]))
    if earspan<6 or span<earspan*1.2 or span>earspan*8:return None
    h,w=shape;box=np.r_[nose-[.85*earspan,1.05*earspan],nose+[.85*earspan,1.05*earspan]]
    if box.min()<0 or box[2]>w or box[3]>h:return None
    if np.linalg.norm(nose-shoulders.mean(0))>span*2:return None
    return tuple(map(float,box)),tuple(map(float,nose)),tuple(map(float,shoulders.reshape(-1)))


def hand_exclusion(hands,shape):
    """Convex hull plus small feather; no assertion of exact hand boundaries."""
    result=np.zeros(shape,np.float32)
    for points in hands:
        p=np.asarray(points,np.float32)
        if p.shape!=(21,2) or not np.isfinite(p).all():raise ValueError('Invalid hand')
        h,w=shape
        if np.abs(p[:,0]).max()>4*w or np.abs(p[:,1]).max()>4*h:raise ValueError('Unbounded hand coordinates')
        hull=cv2.convexHull(np.rint(p).astype(np.int32));area=cv2.contourArea(hull)
        if area<8:continue
        mask=np.zeros(shape,np.uint8);cv2.fillConvexPoly(mask,hull,1)
        radius=max(1,min(8,round(np.sqrt(area)*.08)))
        mask=cv2.dilate(mask,np.ones((radius*2+1,radius*2+1),np.uint8))
        # Full exclusion inside, feather only outside. Never erode known hand.
        distance=cv2.distanceTransform(1-mask,cv2.DIST_L2,cv2.DIST_MASK_PRECISE)
        result=np.maximum(result,(1-smoothstep(0,2,distance)).astype(np.float32))
    return result


def same_person(reference,candidates):
    """Unique local association; face geometry works without visible shoulders.

    Body coordinates refine body-to-body association. A face-only observation
    uses box scale and center, never fabricated shoulder landmarks.
    """
    matches=[]
    for candidate in candidates:
        if reference.shoulders is not None and candidate.shoulders is not None:
            r=np.asarray(reference.shoulders).reshape(2,2);p=np.asarray(candidate.shoulders).reshape(2,2)
            span=np.linalg.norm(r[0]-r[1]);other=np.linalg.norm(p[0]-p[1])
            accepted=(span>0 and .65<=other/span<=1.5 and
                np.linalg.norm(p.mean(0)-r.mean(0))<=.5*span and
                np.linalg.norm(np.array(candidate.nose)-reference.nose)<=.65*span)
        else:
            r=np.asarray(reference.anchor.box if reference.anchor is not None else reference.head)
            p=np.asarray(candidate.anchor.box if candidate.anchor is not None else candidate.head)
            rs=r[2:]-r[:2];ps=p[2:]-p[:2]
            accepted=(np.all(rs>0) and np.all(ps>0) and np.all(ps/rs>=.65) and
                np.all(ps/rs<=1.5) and np.linalg.norm((p[:2]+p[2:]-r[:2]-r[2:])/2)<=.5*min(rs[1],ps[1]))
        if accepted:matches.append(candidate)
    return matches[0] if len(matches)==1 else None


def independent_face_candidates(raw,people,shape):
    """Return isolated confirmed faces not already represented by body evidence.

    Existing global mesh is safe to crop only if another confirmed face does
    not intersect that crop. Ambiguous overlaps abstain, never share an anchor.
    """
    contributors=[f for f in raw['detail']['faces'] if f['status']=='confirmed']
    faces=[f for f in contributors if f['detector_score']>=.8]
    candidates=[]
    for face in faces:
        b=np.asarray(face['box'],float)
        if any(p.anchor is not None and box_iou(b,np.asarray(p.anchor.box))>.5 for p in people):continue
        x0,y0,x1,y1=crop_box(b,shape,1.2)
        region=np.array([x0,y0,x1,y1])
        if any(f is not face and box_iou(region,np.asarray(f['box']))>0 for f in contributors):continue
        support=np.zeros(shape,np.float32);support[y0:y1,x0:x1]=1
        observed=(raw['mesh']*support).astype(np.float32)
        if observed.sum()>=1:candidates.append((tuple(b),support,observed))
    return candidates


def stabilized_source_height(person,neighbors):
    """Symmetric bounded-window size estimate; no history or render-order state.

    Current evidence dominates. Only unique compatible tracks supply neighbors.
    This does not estimate identity or recover an undetected head.
    """
    if not neighbors:return person.source_height
    heights=np.asarray([p.source_height for p in neighbors],float)
    if not np.isfinite(heights).all() or np.any(heights<=0):raise ValueError('Invalid neighboring size')
    return float(.75*person.source_height+.25*np.median(heights))


def capacity_status(detected,limit=4):
    if not isinstance(detected,int) or not isinstance(limit,int) or limit<1 or not 0<=detected<=limit:
        raise ValueError('Invalid detector capacity')
    return dict(limit=limit,returned=detected,capacity_reached=detected==limit,
                coverage='potentially_truncated' if detected==limit else 'not_certified')


class PersonHeadAnalyzer:
    def __init__(self,pose_model,hand_model,skin_model,face_model,detector_model):
        self.pose=None;self.hand=None;self.face=None
        for path,expected in ((pose_model,POSE_SHA),(hand_model,HAND_SHA)):
            if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=expected:raise ValueError('Unknown model bytes')
        import mediapipe as mp
        self.mp=mp
        try:
            self.pose=mp.tasks.vision.PoseLandmarker.create_from_options(mp.tasks.vision.PoseLandmarkerOptions(
                base_options=mp.tasks.BaseOptions(model_asset_path=str(pose_model)),running_mode=mp.tasks.vision.RunningMode.IMAGE,
                num_poses=4,output_segmentation_masks=True,min_pose_detection_confidence=.5,min_pose_presence_confidence=.5))
            self.hand=mp.tasks.vision.HandLandmarker.create_from_options(mp.tasks.vision.HandLandmarkerOptions(
                base_options=mp.tasks.BaseOptions(model_asset_path=str(hand_model)),running_mode=mp.tasks.vision.RunningMode.IMAGE,
                num_hands=4,min_hand_detection_confidence=.5,min_hand_presence_confidence=.5))
            self.face=TurnSkinSegmenter(skin_model,face_model,detector_model)
        except BaseException:
            self.close();raise

    def close(self):
        failure=None
        for name in ('face','hand','pose'):
            obj=getattr(self,name,None)
            if obj is not None:
                try:obj.close()
                except Exception as error:failure=failure or error
                finally:setattr(self,name,None)
        if failure is not None:raise failure

    def _image(self,rgb):return self.mp.Image(image_format=self.mp.ImageFormat.SRGB,data=np.ascontiguousarray(np.rint(rgb*255).astype(np.uint8)))

    def analyze(self,rgb,source_scale_y=1.,allow_approximate=False):
        validate_rgb(rgb);h,w=rgb.shape[:2]
        if rgb.dtype!=np.float32 or min(h,w)<32 or max(h,w)>2048 or h*w>2097152:raise ValueError('Invalid research frame')
        if not np.isfinite(source_scale_y) or source_scale_y<=0:raise ValueError('Invalid source scale')
        raw=self.face.analyze_turn(rgb,minimum_mesh_height=64/source_scale_y)
        small_only=bool(raw['detail']['faces']) and all((f['box'][3]-f['box'][1])*source_scale_y<80 for f in raw['detail']['faces'])
        pose=None if small_only else self.pose.detect(self._image(rgb))
        landmarks_list=[] if pose is None else pose.pose_landmarks
        masks=[] if pose is None else (pose.segmentation_masks or [])
        if len(masks)!=len(landmarks_list):raise ValueError('Pose mask count mismatch')
        people=[];details=[]
        small_faces=[f for f in raw['detail']['faces'] if (f['box'][3]-f['box'][1])*source_scale_y<80 and f['detector_score']>=.8]
        for landmarks,mask in zip(landmarks_list,masks):
            points=np.array([[p.x*w,p.y*h,p.visibility,p.presence] for p in landmarks],np.float32)
            geometry=head_geometry(points,(h,w))
            if geometry is None:details.append(dict(status='head_unconfirmed'));continue
            head,nose,shoulders=geometry
            if any(f['box'][0]<=nose[0]<=f['box'][2] and f['box'][1]<=nose[1]<=f['box'][3] for f in small_faces):continue
            x0,y0,x1,y1=crop_box(np.array(head),(h,w),2.6)
            sample=rgb[y0:y1,x0:x1];hand=self.hand.detect(self._image(sample));hands=[]
            for pts in hand.hand_landmarks:
                hands.append(np.array([[p.x*(x1-x0)+x0,p.y*(y1-y0)+y0] for p in pts],np.float32))
            veto=hand_exclusion(hands,(h,w));person=mask.numpy_view().copy().squeeze()
            if not np.isfinite(person).all() or person.min()<0 or person.max()>1:raise ValueError('Invalid person mask')
            person=np.clip(cv2.resize(person,(w,h)),0,1).astype(np.float32)
            skin=np.zeros((h,w),np.float32);channels=self.face.skin._infer(sample)
            skin[y0:y1,x0:x1]=smoothstep(.45,.85,channels[...,3]);skin*=person
            faces=[f for f in raw['detail']['faces'] if f['status']=='confirmed' and f['detector_score']>=.8
                   and f['box'][0]<=nose[0]<=f['box'][2] and f['box'][1]<=nose[1]<=f['box'][3]]
            anchor=None
            if len(faces)==1:
                b=np.asarray(faces[0]['box']);region=np.zeros((h,w),np.float32)
                bx0,by0,bx1,by1=crop_box(b,(h,w),1.2);region[by0:by1,bx0:bx1]=1
                competitors=[f for f in raw['detail']['faces'] if f is not faces[0] and f['status']=='confirmed'
                             and box_iou(np.array([bx0,by0,bx1,by1]),np.asarray(f['box']))>0]
                observed=raw['mesh']*region
                if not competitors and observed.sum()>=1:anchor=FaceAnchor(tuple(b),observed,exclusion=veto)
            people.append(PersonEvidence(head,nose,shoulders,person,skin,veto,anchor))
            details.append(dict(status='person_head',head=head,nose=nose,shoulders=shoulders,hands=len(hands),anchor=anchor is not None))
        # Face-only path: no shoulder/pose requirement. Reuse the confirmed
        # mesh, but collect hand evidence around each automatic face crop.
        for box,support,observed in independent_face_candidates(raw,people,(h,w)):
            x0,y0,x1,y1=crop_box(np.asarray(box),(h,w),2.6)
            sample=rgb[y0:y1,x0:x1];hand=self.hand.detect(self._image(sample))
            hands=[np.array([[p.x*(x1-x0)+x0,p.y*(y1-y0)+y0] for p in pts],np.float32) for pts in hand.hand_landmarks]
            veto=hand_exclusion(hands,(h,w));skin=np.zeros((h,w),np.float32)
            skin[y0:y1,x0:x1]=smoothstep(.45,.85,self.face.skin._infer(sample)[...,3])
            nose=tuple((np.asarray(box[:2])+box[2:])/2)
            # A pose lacking an anchor at this face is superseded by the
            # stronger confirmed face; retain all its detected hand exclusions.
            displaced=[p for p in people if p.anchor is None and box[0]<=p.nose[0]<=box[2] and box[1]<=p.nose[1]<=box[3]]
            for old in displaced:veto=np.maximum(veto,old.hands)
            people=[p for p in people if not any(p is old for old in displaced)]
            people.append(PersonEvidence(box,nose,None,support,skin*support,veto,FaceAnchor(box,observed,exclusion=veto)))
        # Small faces need only an approximate detected head box. No mesh,
        # shoulder requirement or fine skin classifier in this branch.
        for f in raw['detail']['faces']:
            b=np.asarray(f['box']);height=float((b[3]-b[1])*source_scale_y)
            if not (24<=height<80 and f['detector_score']>=.8):continue
            if any(p.anchor is not None and box_iou(b,np.asarray(p.anchor.box))>.5 for p in people):continue
            region=proposal_region(b,(h,w));center=tuple((b[:2]+b[2:])/2)
            # Overlapping detections cannot share photometric support.
            if any(q is not f and box_iou(b,np.asarray(q['box']))>0 for q in raw['detail']['faces']):continue
            represented=[p for p in people if p.anchor is None and b[0]<=p.nose[0]<=b[2] and b[1]<=p.nose[1]<=b[3]]
            veto=np.maximum.reduce([p.hands for p in represented]) if represented else np.zeros((h,w),np.float32)
            people=[p for p in people if not any(p is old for old in represented)]
            people.append(PersonEvidence(tuple(b),center,None,region,raw['semantic']*region,veto,None,height,True))
        if allow_approximate:
            for f in raw['detail']['faces']:
                b=np.asarray(f['box']);height=float((b[3]-b[1])*source_scale_y)
                if f['detector_score']<.8 or height<24:continue
                if any(box_iou(b,np.asarray(p.anchor.box if p.anchor is not None else p.head))>.2 for p in people):continue
                if any(q is not f and box_iou(b,np.asarray(q['box']))>.1 for q in raw['detail']['faces']):continue
                region=proposal_region(b,(h,w));zero=np.zeros((h,w),np.float32)
                people.append(PersonEvidence(tuple(b),tuple((b[:2]+b[2:])/2),None,region,zero,zero,None,height,True,True))
        people=[replace(p,source_height=float(((p.anchor.box if p.anchor is not None else p.head)[3]-(p.anchor.box if p.anchor is not None else p.head)[1])*source_scale_y)) for p in people]
        all_hands=np.maximum.reduce([p.hands for p in people]) if people else np.zeros((h,w),np.float32)
        final=[]
        for p in people:
            anchor=p.anchor
            if anchor is not None:
                b=anchor.box
                if any(q is not p and b[0]<=q.nose[0]<=b[2] and b[1]<=q.nose[1]<=b[3] for q in people):anchor=None
                else:anchor=replace(anchor,exclusion=all_hands)
            final.append(replace(p,hands=all_hands,anchor=anchor))
        details=[dict(status='face_only' if p.shoulders is None else 'person_head',head=p.head,nose=p.nose,shoulders=p.shoulders,anchor=p.anchor is not None,source_height=p.source_height,light_region=p.light_region,hand_pixels=int((p.hands>.5).sum())) for p in final]
        return final,dict(poses=len(landmarks_list),capacity=capacity_status(len(landmarks_list)),face_capacity=capacity_status(len(raw['detail']['faces']),MAX_FACES),people=details,raw_faces=raw['detail'])


def recover_coarse_regions(images,people,center,cuts,appearance=False):
    """Bridge <=2 missing coarse observations using boxes and patch appearance.

    Two independent endpoint detections; never a recovered seed. Approximate
    color matching is not a skin or occlusion classifier.
    """
    result=np.zeros(images[center].shape[:2],np.float32);records=[];claimed=[]
    def box(p):return np.asarray(p.anchor.box if p.anchor is not None else p.head,float)
    def patch(rgb,b):
        size=b[2:]-b[:2];middle=(b[:2]+b[2:])/2
        sample=cv2.getRectSubPix(rgb,(max(2,round(size[0])),max(2,round(size[1]))),tuple(map(float,middle)))
        return cv2.resize(sample,(16,16))
    for lo in range(center-1,max(-1,center-3),-1):
        for left in people[lo]:
            if not (left.light_region or (left.anchor is not None and left.anchor.confirmed)) or (not appearance and left.source_height>=80):continue
            if any(box_iou(box(left),b)>.2 for b in claimed):continue
            for hi in range(center+1,min(len(images),center+3)):
                if hi-lo>3 or any(cuts[lo:hi]):continue
                right=same_person(left,people[hi])
                if right is None or same_person(right,people[lo]) is not left or (not appearance and right.source_height>=80):continue
                if not (right.light_region or (right.anchor is not None and right.anchor.confirmed)):continue
                t=(center-lo)/(hi-lo);b=(1-t)*box(left)+t*box(right)
                # An observed current person or an ambiguous intermediate
                # proposal in this region must not receive a second mask.
                if any(any(box_iou(b,box(p))>.1 for p in people[j]) for j in range(lo+1,hi)):continue
                current=patch(images[center],b)
                error=max(float(np.abs(current-patch(images[j],box(p))).mean()) for j,p in ((lo,left),(hi,right)))
                if error>.08:continue
                height=(1-t)*left.source_height+t*right.source_height
                zero=np.zeros_like(result);veto=zero.copy()
                for endpoint in (() if appearance else (left,right)):
                    known=endpoint.hands
                    if endpoint.anchor is not None and endpoint.anchor.exclusion is not None:known=np.maximum(known,endpoint.anchor.exclusion)
                    source=box(endpoint);scale=(b[2:]-b[:2])/(source[2:]-source[:2]);offset=b[:2]-scale*source[:2]
                    transform=np.array([[scale[0],0,offset[0]],[0,scale[1],offset[1]]],np.float32)
                    veto=np.maximum(veto,cv2.warpAffine(known,transform,(result.shape[1],result.shape[0]),flags=cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT))
                p=PersonEvidence(tuple(b),tuple((b[:2]+b[2:])/2),None,zero,zero,veto,None,height,True)
                if appearance:
                    region=proposal_region(b,result.shape)
                    mask=.65*float(smoothstep(24,32,height))*cv2.GaussianBlur(region,(0,0),max(.8,(b[3]-b[1])*.06))*region
                else:mask=size_adaptive_region(p,zero)
                result=np.maximum(result,mask);claimed.append(b)
                records.append(dict(status='coarse_gap',endpoints=[lo,hi],source_height=height,appearance_error=error));break
    return result,records


def automatic_window(images,people,center,cuts):
    """All anchor choices are automatic inside radius2; no saved identity state."""
    n=len(images)
    if not 3<=n<=5 or len(people)!=n or len(cuts)!=n-1 or not 0<=center<n:raise ValueError('Invalid automatic window')
    shape=images[center].shape[:2]
    for rgb,entries in zip(images,people):
        validate_rgb(rgb)
        if (rgb.dtype!=np.float32 or rgb.shape!=images[center].shape or len(entries)>MAX_FACES+4
                or min(shape)<32 or max(shape)>2048 or shape[0]*shape[1]>2097152):raise ValueError('Invalid frame/people')
        for person in entries:
            for m in (person.person,person.skin,person.hands):
                if m.shape!=shape or m.dtype!=np.float32 or not np.isfinite(m).all() or m.min()<0 or m.max()>1:raise ValueError('Invalid person evidence')
            geometry=np.asarray((*person.head,*person.nose,*(person.shoulders or ())),float)
            if geometry.shape!=((6,) if person.shoulders is None else (10,)) or not np.isfinite(geometry).all():raise ValueError('Invalid person geometry')
            b=np.asarray(person.head)
            if b.min()<0 or b[2]>shape[1] or b[3]>shape[0] or np.any(b[2:]<=b[:2]):raise ValueError('Invalid head')
            if person.anchor is not None:
                m=person.anchor.mask
                if m.shape!=shape or m.dtype!=np.float32 or not np.isfinite(m).all() or m.min()<0 or m.max()>1:raise ValueError('Invalid anchor')
                b=np.asarray(person.anchor.box,float)
                if b.shape!=(4,) or not np.isfinite(b).all() or b.min()<0 or b[2]>shape[1] or b[3]>shape[0] or np.any(b[2:]<=b[:2]):raise ValueError('Invalid anchor box')
                veto=person.anchor.exclusion
                if veto is not None and (veto.shape!=shape or not np.isfinite(veto).all() or veto.min()<0 or veto.max()>1):raise ValueError('Invalid anchor exclusion')
    result=np.zeros(shape,np.float32);records=[]
    all_hands=np.maximum.reduce([p.hands for p in people[center]]) if people[center] else np.zeros(shape,np.float32)
    for person in people[center]:
        # Closest mutually associated raw observations on each side; no state
        # or traversal through cuts/ambiguous people. Do not use processed masks.
        neighbors=[]
        for direction in (-1,1):
            for step in (1,2):
                j=center+direction*step
                if j<0 or j>=n or cuts[min(j,j-direction)]:break
                candidate=same_person(person,people[j])
                if candidate is None:
                    if people[j]:break
                    continue
                if same_person(candidate,people[center]) is not person:break
                neighbors.append((j,candidate));break
        effective_height=stabilized_source_height(person,[p for _,p in neighbors])
        policy_person=replace(person,source_height=effective_height)
        if person.light_region:
            result=np.maximum(result,size_adaptive_region(policy_person,np.zeros(shape,np.float32)))
            records.append(dict(status='light_region',source_height=person.source_height,effective_height=effective_height,tracked_neighbors=len(neighbors),face_geometry_tracking=False));continue
        bracketing=len(neighbors)==2 and neighbors[0][0]<center<neighbors[1][0]
        if bracketing:
            hand_left=structural_warp(images[center],images[neighbors[0][0]],neighbors[0][1].hands)
            hand_right=structural_warp(images[center],images[neighbors[1][0]],neighbors[1][1].hands)
            all_hands=np.maximum(all_hands,np.minimum(hand_left,hand_right))
        headmask=np.zeros(shape,np.float32);x0,y0,x1,y1=crop_box(np.array(person.head),shape,1.2);headmask[y0:y1,x0:x1]=1
        if person.anchor is not None:
            if not person.anchor.confirmed:continue
            veto=person.anchor.exclusion if person.anchor.exclusion is not None else np.zeros(shape,np.float32)
            current=np.minimum(np.minimum(person.anchor.mask,person.person),1-veto)*headmask
            checked=False
            if bracketing and all(p.anchor is not None for _,p in neighbors):
                gates=[structural_warp(images[center],images[j],p.anchor.mask) for j,p in neighbors]
                current=np.minimum(current,np.minimum(*gates));checked=True
            if not checked:current*=.35
            current=size_adaptive_region(policy_person,current)
            current=np.minimum(current,1-veto)
            result=np.maximum(result,current);records.append(dict(status='current_anchor',source_height=person.source_height,effective_height=effective_height,tracked_neighbors=len(neighbors),two_sided_verified=checked,unchecked_strength_cap=None if checked else .35));continue
        matched=[];ambiguous=[]
        for index,candidates in enumerate(people):
            candidate=same_person(person,candidates)
            if candidate is not None and same_person(candidate,people[center]) is not person:candidate=None
            ambiguous.append(bool(candidates) and candidate is None)
            matched.append([candidate.anchor] if candidate is not None and candidate.anchor is not None else [])
        left=[i for i in range(max(0,center-2),center) if matched[i]]
        right=[i for i in range(center+1,min(n,center+3)) if matched[i]]
        if ambiguous[center] or (left and right and any(ambiguous[max(left):min(right)+1])):
            records.append(dict(status='abstained',reason='ambiguous_person_interval'));continue
        transferred,detail=transfer_blurred_face(images,matched,center,cuts,person.skin,np.zeros(shape,np.float32))
        # Current person/head confines transferred masks; body evidence alone
        # must never create an ellipse or claim visible skin.
        headmask=np.zeros(shape,np.float32);x0,y0,x1,y1=crop_box(np.array(person.head),shape,1.2);headmask[y0:y1,x0:x1]=1
        transferred=np.minimum(transferred,person.person)*headmask
        # Coarse regions are already handled independently above. Recovery
        # must never create a new spot from absent correspondence.
        result=np.maximum(result,np.minimum(transferred,size_adaptive_region(policy_person,transferred)));records.append(detail)
    recovered,trace=recover_coarse_regions(images,people,center,cuts)
    result=np.maximum(result,recovered);records.extend(trace)
    return np.minimum(result,1-all_hands),records
