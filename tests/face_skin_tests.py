"""Independent mask-boundary tests; real learned-model quality is separate."""
import sys
from pathlib import Path
from types import SimpleNamespace
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import numpy as np
import face_skin_analysis as f


def main():
    shape=(96,96)
    theta=np.arange(16)*2*np.pi/16
    points=np.tile([48.,48.],(478,1))
    points[:16]=np.c_[48+30*np.cos(theta),48+35*np.sin(theta)]
    points[16:20]=[[37,35],[43,35],[43,41],[37,41]]
    support=f.face_support(points,shape,list(range(16)),[list(range(16,20))])
    assert support[48,48]==1 and support[38,40]==0
    assert not support[:10].any() and not support[:, :15].any()
    assert np.all((support>=0)&(support<=1)) and np.any((support>0)&(support<1))
    assert not f.face_support(points+100,shape,list(range(16)),[]).any()
    broken=points.copy();broken[100,0]=np.nan
    try:f.face_support(broken,shape,list(range(16)),[]);raise AssertionError('NaN accepted')
    except ValueError:pass
    boxes=[(np.array([20,20,50,60]),.9),(np.array([21,20,51,60]),.8),(np.array([65,20,85,55]),.85)]
    chosen=f.select_boxes(boxes,shape)
    assert len(chosen)==2 and chosen[0][1]==.9
    assert [x[1] for x in chosen]==[x[1] for x in f.select_boxes(boxes[::-1],shape)]
    assert not f.select_boxes([(np.array([-1,20,50,60]),.9)],shape)
    try:f.select_boxes([(np.array([0,0,np.inf,30]),.9)],shape);raise AssertionError('Inf accepted')
    except ValueError:pass
    # Independent constant-texture window isolates support clamping from flow.
    image=np.full((*shape,3),.5,np.float32)
    masks=[np.ones(shape,np.float32)]*3
    center=support.copy();center[45:55,45:55]=0 # Newly occluding central hole.
    result=f.stabilize_face_mask([image]*3,masks,[support,center,support],1,[False,False])
    assert not result[center==0].any() and np.all(result<=center)
    zero=np.zeros(shape,np.float32)
    assert not f.stabilize_face_mask([image]*3,masks,[support,zero,support],1,[False]*2).any()
    actual=masks.copy();actual[1]=center*.5
    assert np.array_equal(f.stabilize_face_mask([image]*3,actual,[support,center,support],1,[True]*2),center*.5)
    assert np.array_equal(result,f.stabilize_face_mask([image]*3,masks,[support,center,support],1,[False,False]))
    # No face proposals: no invocation of either downstream learned model.
    candidate=object.__new__(f.FaceSkinSegmenter)
    candidate.detector=SimpleNamespace(setInputSize=lambda _:None,detect=lambda _:(0,None))
    mask,region,detail=candidate.analyze_with_support(image)
    assert not mask.any() and not region.any() and detail['face_count']==0
    print('PASS face geometry, feature exclusion, inward feather, NMS, absent-face exact zero, cuts and current-support occlusion boundary')

if __name__=='__main__':main()
