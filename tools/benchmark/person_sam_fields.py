"""Per-seed SAM2 geometry proposals bound to automatic person tracks, research only."""
import json,hashlib
from pathlib import Path
import cv2,numpy as np
from scene_analysis import smooth


def blend_face_field(base,target,trust,ceiling):
    values=[np.asarray(v,np.float32) for v in (base,target,trust,ceiling)]
    if any(v.shape!=values[0].shape or not np.isfinite(v).all() or v.min()<-1e-6 or v.max()>1+1e-6 for v in values):raise ValueError('Invalid face probability blend')
    base,target,trust,ceiling=[np.clip(v,0,1) for v in values]
    return np.clip(np.minimum((1-trust)*base+trust*target,ceiling),0,1).astype(np.float32)


def apply_sam_faces(frames,ids,directory,source_cases):
    directory=Path(directory);root=directory.parent;sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
    evidence=json.loads((directory/'evidence.json').read_text());manifest=json.loads((root/'manifest.json').read_text())
    if not evidence.get('per_object_output') or sha(root/'manifest.json')!=evidence['manifest_sha256']:raise ValueError('SAM object evidence mismatch')
    if [(r['name'],r['sha256']) for r in manifest['cases']]!=[(r['name'],r['sha256']) for r in source_cases]:raise ValueError('SAM source sequence mismatch')
    def load(direction,index):
        path=directory/f'{direction}-{index:03}-objects.npy'
        if path.name not in evidence['output_sha256'] or sha(path)!=evidence['output_sha256'][path.name]:raise ValueError('Changed SAM object prediction')
        value=np.load(path,allow_pickle=False)
        if value.ndim!=3 or not np.isfinite(value).all() or value.min()<0 or value.max()>1:raise ValueError('Invalid SAM probability')
        return value
    mapping={};amplitudes={};anchors={r['direction']:r for r in manifest['anchors']}
    for direction,anchor in anchors.items():
        seeds=np.load(root/(direction+'-seed.npy'),allow_pickle=False)
        if sha(root/(direction+'-seed.npy'))!=anchor['seed_sha256'] or seeds.ndim!=3:raise ValueError('Changed SAM seeds')
        index=anchor['index'];active=[(o,t) for o,t in zip(frames[index],ids[index]) if t is not None]
        scores=np.array([[float((seed&(o['mask']>.1)).sum()/max(1,seed.sum())) for o,t in active] for seed in seeds])
        if scores.shape!=(len(seeds),len(seeds)):raise ValueError('SAM/person count disagreement')
        order=scores.argmax(1)
        if len(set(order.tolist()))!=len(seeds):raise ValueError('Ambiguous seed/person association')
        mapping[direction]={}
        for k,column in enumerate(order):
            if scores[k,column]<.5 or (len(order)>1 and scores[k,column]-np.delete(scores[k],column).max()<.25):raise ValueError('Weak seed association')
            o,t=active[column];mapping[direction][t]=k;amplitudes.setdefault(t,[]).append(float(np.median(o['mask'][seeds[k]])))
    if set(mapping['forward'])!=set(mapping['backward']):raise ValueError('SAM endpoint identity disagreement')
    # Each label must roundtrip into its own opposite seed, not merely their union.
    for direction,other in [('forward','backward'),('backward','forward')]:
        seeds=np.load(root/(other+'-seed.npy'),allow_pickle=False);prediction=load(direction,anchors[other]['index'])
        for track,index in mapping[direction].items():
            seed=seeds[mapping[other][track]];mask=prediction[index]>.5;intersection=int((seed&mask).sum());union=int((seed|mask).sum())
            if intersection/max(1,int(seed.sum()))<.5 or intersection/max(1,union)<.35:raise ValueError('Per-object SAM roundtrip failed')
    start,end=anchors['forward']['index'],anchors['backward']['index'];output=[]
    for index,(observations,tracks) in enumerate(zip(frames,ids)):
        if index<start or index>end:output.append(observations);continue
        forward,backward=load('forward',index),load('backward',index);current=[]
        phase=min(1.,max(0.,min(index-start,end-index)/2));fade=phase*phase*(3-2*phase)
        for o,t in zip(observations,tracks):
            updated=dict(o,mask=o['mask'].copy())
            if t in mapping['forward'] and 'face_ceiling' in o:
                a=forward[mapping['forward'][t]];b=backward[mapping['backward'][t]]
                agreed=cv2.GaussianBlur(np.minimum(a,b),(0,0),1.2);trust=cv2.GaussianBlur((1-smooth(.15,.5,np.abs(a-b))).astype(np.float32),(0,0),1.2)*fade
                target=np.minimum(agreed*float(np.mean(amplitudes[t])),o['face_ceiling'])
                x0,y0,x1,y1=o['box'];yy,xx=np.mgrid[:a.shape[0],:a.shape[1]];inside=(xx>=x0)&(xx<x1)&(yy>=y0)&(yy<y1)
                mixed=blend_face_field(o['mask'],target,trust,o['face_ceiling']);updated['mask'][inside]=mixed[inside]
            current.append(updated)
        output.append(current)
    return output,sha(directory/'evidence.json')
