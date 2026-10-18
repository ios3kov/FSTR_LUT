"""Bounded offline person association; explicit shot input, no render-time history.

Face confirmation is supplied by the existing detector, never by skin color alone.
Ambiguous matches split references rather than transfer a person's tone by force.
"""
import numpy as np


def box_iou(a,b):
    lo=np.maximum(a[:2],b[:2]);hi=np.minimum(a[2:],b[2:]);area=float(np.maximum(hi-lo,0).prod())
    return area/max(1e-12,float(np.prod(a[2:]-a[:2])+np.prod(b[2:]-b[:2]))-area)


def observation(box,mask,rgb,confidence=1.,descriptor_support=None):
    b=np.asarray(box,float);m=np.asarray(mask,np.float32)
    if b.shape!=(4,) or not np.isfinite(b).all() or np.any(b[2:]<=b[:2]):raise ValueError('Invalid face box')
    if rgb.ndim!=3 or rgb.shape[2]!=3 or m.shape!=rgb.shape[:2] or not np.isfinite(rgb).all() or not np.isfinite(m).all() or m.min()<0 or m.max()>1:raise ValueError('Invalid observation pixels')
    if not np.isfinite(confidence) or not 0<=confidence<=1:raise ValueError('Invalid observation confidence')
    descriptor_mask=m if descriptor_support is None else np.asarray(descriptor_support,np.float32)
    if descriptor_mask.shape!=m.shape or not np.isfinite(descriptor_mask).all() or descriptor_mask.min()<0 or descriptor_mask.max()>1:raise ValueError('Invalid independent geometry support')
    support=descriptor_mask>.25
    if support.sum()<8:return None
    colors=rgb[support];y=colors@np.array([.2126,.7152,.0722]);valid=(y>1e-5)&(colors.min(1)>0)&(colors.max(1)<1)
    if valid.sum()<8:return None
    chroma=np.median(colors[valid]/y[valid,None],axis=0)
    return dict(box=b,mask=m,descriptor=chroma,confidence=float(confidence))


def associate(frames,cut_before=(),max_gap=2):
    if max_gap not in (1,2):raise ValueError('Only bounded one/two-frame association')
    cuts=set(cut_before)
    if any(not isinstance(i,int) or i<0 or i>=len(frames) for i in cuts):raise ValueError('Invalid cut')
    result=[];tracks={};next_id=0
    for frame,observations in enumerate(frames):
        if frame in cuts:tracks={}
        if len(observations)>16:raise ValueError('Face budget')
        for o in observations:
            b=np.asarray(o['box']);d=np.asarray(o['descriptor'])
            if b.shape!=(4,) or d.shape!=(3,) or not np.isfinite(b).all() or not np.isfinite(d).all() or np.any(b[2:]<=b[:2]):raise ValueError('Invalid association observation')
        active=sorted(k for k,v in tracks.items() if 1<=frame-v['frame']<=max_gap)
        # Overlapping heads are not safe evidence for handing off a color reference.
        crowded=[any(j!=i and box_iou(o['box'],other['box'])>.12 for j,other in enumerate(observations)) for i,o in enumerate(observations)]
        blocked=set()
        for k in active:
            old=tracks[k]['observation']['box'];center=(old[:2]+old[2:])/2
            for o,is_crowded in zip(observations,crowded):
                b=o['box'];scale=np.maximum(old[2:]-old[:2],b[2:]-b[:2])
                if is_crowded and np.linalg.norm((center-(b[:2]+b[2:])/2)/scale)<=1.5:blocked.add(k)
        for k in blocked:del tracks[k]
        active=[k for k in active if k not in blocked]
        costs=np.full((len(observations),len(active)),np.inf)
        for i,o in enumerate(observations):
            if crowded[i]:continue
            b=o['box'];size=b[2:]-b[:2];center=(b[:2]+b[2:])/2
            for j,k in enumerate(active):
                old=tracks[k];a=old['observation']['box'];gap=frame-old['frame'];old_size=a[2:]-a[:2]
                spatial=np.linalg.norm((center-(a[:2]+a[2:])/2)/np.maximum(size,old_size))/gap
                scale=np.max(np.abs(np.log(size/old_size)))
                color=np.linalg.norm(o['descriptor']-old['observation']['descriptor'])
                if spatial<=.75 and scale<=.5 and color<=.45:costs[i,j]=spatial+.3*scale+.5*color
        assigned=[];updates={};ambiguous_tracks=set()
        for i,o in enumerate(observations):
            if crowded[i]:
                assigned.append(None);continue
            options=np.argsort(costs[i])
            match=None
            if len(options) and np.isfinite(costs[i,options[0]]):
                j=int(options[0]);col=np.sort(costs[:,j]);row=np.sort(costs[i])
                unique_row=len(row)==1 or row[1]-row[0]>.2
                unique_col=len(col)==1 or col[1]-col[0]>.2
                if unique_row and unique_col and int(np.argmin(costs[:,j]))==i:match=active[j]
                else:ambiguous_tracks.update(active[k] for k in range(len(active)) if np.isfinite(costs[i,k]))
            if match is None:
                match=next_id;next_id+=1
            assigned.append(match);updates[match]=dict(frame=frame,observation=o)
        tracks={k:v for k,v in tracks.items() if frame-v['frame']<max_gap and k not in ambiguous_tracks};tracks.update(updates);result.append(assigned)
    return result


def ownership(fields):
    a=np.asarray(fields,np.float32)
    if a.ndim!=3 or len(a)>16 or not np.isfinite(a).all() or a.min()<0 or a.max()>1:raise ValueError('Invalid person fields')
    # Continuous soft ambiguity rejection; no winning hard label edge.
    if len(a)==1:return a.copy()
    strongest=np.sort(a,axis=0);margin=np.divide(strongest[-1]-strongest[-2],strongest[-1],out=np.ones_like(strongest[-1]),where=strongest[-1]>0)
    t=np.clip(margin/.5,0,1);confidence=t*t*(3-2*t)
    return (a/np.maximum(1,a.sum(0))[None]*confidence[None]).astype(np.float32)


def attach_body_skin(observations,poses,parts,linear):
    """Use a uniquely face-associated person mask AND explicit body-skin material.

    A person silhouette or similar color alone never becomes exposed skin.
    Ambiguous person/face association leaves body unknown, not shared between faces.
    """
    shape=linear.shape[:2];poses=np.asarray(poses,np.float32);parts=np.asarray(parts,np.float32)
    if poses.ndim!=3 or poses.shape[1:]!=shape or parts.shape!=(*shape,6) or not np.isfinite(poses).all() or not np.isfinite(parts).all() or poses.min(initial=0)<0 or poses.max(initial=0)>1 or parts.min()<0 or parts.max()>1 or np.max(parts.sum(-1))>1.00001:raise ValueError('Invalid body evidence')
    result=[dict(o,mask=o['mask'].copy(),ceiling=o.get('ceiling',o['mask']).copy()) for o in observations]
    scores=np.zeros((len(observations),len(poses)))
    for i,o in enumerate(observations):
        for j,person in enumerate(poses):scores[i,j]=float((o['mask']*person).sum()/max(1e-12,float(o['mask'].sum())))
    for j,person in enumerate(poses):
        if not len(observations):continue
        order=np.argsort(scores[:,j])[::-1];i=int(order[0]);best=scores[i,j];second=scores[order[1],j] if len(order)>1 else 0
        if best<.6 or best-second<.25:continue
        # Multiple overlapping pose proposals for this face are not independent evidence.
        if j!=int(np.argmax(scores[i])):continue
        row=np.sort(scores[i])[::-1]
        if len(row)>1 and row[0]-row[1]<.25:continue
        y=linear@np.array([.2126,.7152,.0722]);chroma=linear/np.maximum(y[...,None],1e-6)
        distance=np.linalg.norm(chroma-observations[i]['descriptor'],axis=-1)
        t=np.clip((distance-.3)/.5,0,1);affinity=1-t*t*(3-2*t)
        clothing=np.clip((parts[...,4]-.3)/.4,0,1);veto=1-clothing*clothing*(3-2*clothing)
        body=parts[...,2]*person*affinity*veto
        result[i]['mask']=np.maximum(result[i]['mask'],body.astype(np.float32))
        non_skin=np.clip((parts[...,0]+parts[...,1]+parts[...,4]-.6)/.3,0,1);permission=person*affinity*(1-non_skin*non_skin*(3-2*non_skin))
        result[i]['ceiling']=np.maximum(result[i]['ceiling'],permission.astype(np.float32))
    return result


def stabilize_fields(images,frames,ids,cut_before=()):
    """Existing explicit ±2 flow/appearance stabilizer, channels bound to track IDs.

    Raw observations only. Missing current people are not invented or recursively fed back.
    """
    from scene_analysis import stabilize_window
    if not len(images)==len(frames)==len(ids) or not 1<=len(frames)<=256:raise ValueError('Explicit frame/identity window required')
    if any(len(o)!=len(t) or len(set(i for i in t if i is not None))!=len([i for i in t if i is not None]) for o,t in zip(frames,ids)):raise ValueError('Ambiguous per-frame identity input')
    cuts=set(cut_before);raw=[]
    for observations,tracks in zip(frames,ids):
        active=[(o,t) for o,t in zip(observations,tracks) if t is not None]
        fields=ownership(np.stack([o['mask'] for o,t in active])) if active else []
        raw.append({t:m for (o,t),m in zip(active,fields)})
    output=[]
    for center,(observations,tracks) in enumerate(zip(frames,ids)):
        current=sorted(raw[center]);result={};boosted={}
        for start in range(0,len(current),8):
            group=current[start:start+8];window={}
            for offset in (-2,-1,0,1,2):
                j=center+offset
                if j<0 or j>=len(frames) or any(k in cuts for k in range(min(j,center)+1,max(j,center)+1)):continue
                fields=np.zeros((*images[center].shape[:2],8),np.float32)
                for channel,track in enumerate(group):
                    if track in raw[j]:fields[...,channel]=raw[j][track]
                window[offset]=(images[j],fields)
            stable=stabilize_window(window)
            boost=two_sided_confidence(window) if any('ceiling' in o for o in observations) else np.zeros_like(stable)
            stable=np.maximum(stable,boost)
            for channel,track in enumerate(group):result[track]=stable[...,channel];boosted[track]=boost[...,channel]
        output.append([dict(o,mask=np.minimum(result[t],np.maximum(o.get('ceiling',o['mask']),np.minimum(o.get('recovery_ceiling',o.get('ceiling',o['mask'])),boosted[t]))) if t is not None else np.zeros_like(o['mask'])) for o,t in zip(observations,tracks)])
    return output


def two_sided_confidence(window):
    """Restore unknown confidence only when both raw temporal sides agree.

    This returns a proposal, never overrides caller's current material/geometry veto.
    """
    import cv2
    from scene_analysis import smooth
    image,current=window[0];h,w=image.shape[:2];grid=np.stack(np.meshgrid(np.arange(w),np.arange(h)),-1).astype(np.float32)
    gray=lambda v:np.rint(cv2.cvtColor(v,cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
    warped={}
    for offset in (-2,-1,1,2):
        direction=-1 if offset<0 else 1;distance=abs(offset)
        if offset not in window or any(direction*k not in window for k in range(1,distance+1)):continue
        other,mask=window[offset]
        forward=cv2.calcOpticalFlowFarneback(gray(image),gray(other),None,.5,3,15,3,5,1.2,0)
        backward=cv2.calcOpticalFlowFarneback(gray(other),gray(image),None,.5,3,15,3,5,1.2,0)
        xy=grid+forward;remap=lambda v:cv2.remap(v,xy[...,0],xy[...,1],cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
        valid=(xy[...,0]>=0)&(xy[...,0]<=w-1)&(xy[...,1]>=0)&(xy[...,1]<=h-1)
        trust=valid*(1-smooth(.5,2.,np.linalg.norm(forward+remap(backward),axis=-1)))*(1-smooth(.025,.10,np.abs(image-remap(other)).mean(-1)))
        warped[offset]=remap(mask)*trust[...,None]
    result=np.zeros_like(current)
    # At most two missing frames: -2/+2 would bridge a forbidden three-frame gap.
    for left,right in ((-1,1),(-1,2),(-2,1)):
        if left not in warped or right not in warped:continue
        a,b=warped[left],warped[right];agreement=1-smooth(.10,.40,np.abs(a-b))
        result=np.maximum(result,np.minimum(a,b)*agreement)
    return result.astype(np.float32)


def mean_face_fields(raw_frames,render_frames,ids):
    """One robust face-local field per explicit track/shot, not a final-RGB average.

    Weak/missing skin observations do not vote as known non-skin. Current geometry
    and exclusion permission remain authoritative. No cross-person pooled template.
    """
    import cv2
    if not len(raw_frames)==len(render_frames)==len(ids):raise ValueError('Face mean window mismatch')
    size=64;uv=np.arange(size,dtype=np.float32)+.5;gx,gy=np.meshgrid(uv,uv);samples={}
    for observations,tracks in zip(raw_frames,ids):
        for o,t in zip(observations,tracks):
            if t is None or 'face_geometry' not in o:continue
            x0,y0,x1,y1=o['box'];xyx=(x0+gx*(x1-x0)/size).astype(np.float32);xyy=(y0+gy*(y1-y0)/size).astype(np.float32)
            mask=cv2.remap(o['mask'],xyx,xyy,cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
            geometry=cv2.remap(o['face_geometry'],xyx,xyy,cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
            samples.setdefault(t,[]).append((mask,(geometry>.1)&(mask>.03)))
    templates={}
    rim=np.minimum.reduce([gx/size,1-gx/size,gy/size,1-gy/size]);u=np.clip(rim/.06,0,1);feather=u*u*(3-2*u)
    for t,observations in samples.items():
        fields=np.stack([m for m,v in observations]);valid=np.stack([v for m,v in observations]);count=valid.sum(0)
        ordered=np.sort(np.where(valid,fields,np.inf),axis=0);lo=np.floor(count*.1).astype(int);hi=count-lo;indices=np.arange(len(ordered))[:,None,None]
        use=(indices>=lo)&(indices<hi)&np.isfinite(ordered);total=np.where(use,ordered,0).sum(0)
        templates[t]=(np.divide(total,np.maximum(1,hi-lo))*feather).astype(np.float32)
    result=[]
    for observations,tracks in zip(render_frames,ids):
        current=[]
        for o,t in zip(observations,tracks):
            updated=dict(o,mask=o['mask'].copy())
            if t in templates and 'face_geometry' in o:
                h,w=o['mask'].shape;yy,xx=np.mgrid[:h,:w].astype(np.float32);x0,y0,x1,y1=o['box']
                mx=((xx-x0)*size/(x1-x0)-.5).astype(np.float32);my=((yy-y0)*size/(y1-y0)-.5).astype(np.float32)
                field=cv2.remap(templates[t],mx,my,cv2.INTER_LINEAR,borderMode=cv2.BORDER_CONSTANT)
                inside=(xx>=x0)&(xx<x1)&(yy>=y0)&(yy<y1)
                ceiling=o.get('face_ceiling',o['face_geometry'])
                updated['mask'][inside]=np.minimum(field,ceiling)[inside]
            current.append(updated)
        result.append(current)
    return result
