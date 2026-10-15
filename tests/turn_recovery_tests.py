"""Behavior tests with known translating texture, not learned-model accuracy."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import cv2,numpy as np
from turn_skin_recovery import recover_short_gap,checked_warp,proposal_region,combine_turn_window


def main():
    rng=np.random.default_rng(73);h=w=128
    texture=cv2.GaussianBlur(rng.uniform(.1,.9,(h,w,3)).astype(np.float32),(3,3),.5)
    original=np.zeros((h,w),np.float32);original[40:85,40:85]=1
    images=[];truth=[]
    for i in range(5):
        transform=np.array([[1,0,2*i],[0,1,i]],np.float32)
        images.append(cv2.warpAffine(texture,transform,(w,h),borderMode=cv2.BORDER_REFLECT))
        truth.append(cv2.warpAffine(original,transform,(w,h)))
    def trial(missing,center,cuts=None,counts=None,evidence=None,frames=None,proposals=None):
        masks=[m.copy() for m in truth];facecounts=[1]*5
        for j in missing:masks[j][:]=0;facecounts[j]=0
        return recover_short_gap(images if frames is None else frames,masks,truth if evidence is None else evidence,
            facecounts if counts is None else counts,[1]*5 if proposals is None else proposals,center,[False]*4 if cuts is None else cuts)
    for missing in ([2],[1,2],[2,3]):
        for center in missing:
            mask,info=trial(missing,center)
            assert info['status']=='temporal_recovery',info
            assert mask[50:75,50:75].mean()>.75
            assert not mask[truth[center]==0].any()
            again,_=trial(missing,center);assert np.array_equal(mask,again)
    for missing,center in [([1,2,3],2),([0,1,2],1),([2,3,4],3)]:
        mask,info=trial(missing,center);assert not mask.any(),info
    for cuts in ([False,True,False,False],[False,False,True,False]):
        mask,_=trial([2],2,cuts=cuts);assert not mask.any()
    mask,_=trial([2],2,proposals=[1,1,0,1,1]);assert not mask.any()
    mask,_=trial([2],2,proposals=[1,1,2,1,1]);assert not mask.any()
    mask,_=trial([2],2,counts=[1,2,0,2,1]);assert not mask.any()
    evidence=[m.copy() for m in truth];evidence[2][:]=0
    mask,_=trial([2],2,evidence=evidence);assert not mask.any()
    # Semantic rejection on a newly occluding patch must be exact; independent
    # appearance rejection also rejects an occluder with different RGB.
    evidence=[m.copy() for m in truth];evidence[2][50:70,50:70]=0
    mask,_=trial([2],2,evidence=evidence);assert not mask[50:70,50:70].any()
    occluded=[im.copy() for im in images];occluded[2][35:95,35:100]=[1,0,1]
    mask,_=trial([2],2,frames=occluded);assert not mask.any()
    # Uniform colors have no reliable motion correspondence.
    flat=[np.full_like(texture,.5)]*5
    mask,_=trial([2],2,frames=flat);assert not mask.any()
    # Wrong-face current proposal is elsewhere despite good motion.
    wrong=[m.copy() for m in truth];wrong[2][:]=0;wrong[2][5:20,5:20]=1
    mask,_=trial([2],2,evidence=wrong);assert not mask.any()
    bad=[m.copy() for m in truth];bad[2][0,0]=np.nan
    try:trial([2],2,evidence=bad);raise AssertionError('Invalid evidence accepted')
    except ValueError:pass
    warped=checked_warp(images[2],images[1],truth[1]);assert warped[50:75,50:75].mean()>.9
    ellipse=proposal_region([20,20,80,100],(128,128));assert ellipse[60,50]==1 and not ellipse[20,20]
    assert np.any((ellipse>0)&(ellipse<1))
    # Complete pipeline: source observations outside radius2 cannot affect output.
    sequence=[texture.copy()]*2+images+[texture.copy()]*2
    seeds=[original.copy()]*2+[m.copy() for m in truth]+[original.copy()]*2
    seeds[4][:]=0
    supports=[m.copy() for m in seeds];zero_profiles=[np.zeros_like(original) for _ in sequence]
    semantics=[original.copy()]*2+truth+[original.copy()]*2
    counts=[1]*9;counts[4]=0
    values,info=combine_turn_window(sequence,seeds,supports,zero_profiles,semantics,semantics,counts,[1]*9,4,[False]*8)
    assert values['smart'][50:75,50:75].mean()>.75 and info['status']=='temporal_recovery'
    sequence[1]=np.full_like(texture,np.nan);sequence[7]=np.zeros_like(texture)
    sequence[0]=np.zeros_like(texture);sequence[8]=np.zeros_like(texture)
    seeds[1]=np.ones_like(original);seeds[7]=np.zeros_like(original)
    counts[1]=2;counts[7]=2
    repeat,_=combine_turn_window(sequence,seeds,supports,zero_profiles,semantics,semantics,counts,[1]*9,4,[False]*8)
    assert all(np.array_equal(values[key],repeat[key]) for key in values)
    assert not seeds[4].any()  # Never overwrite source masks with recovered output.
    print('PASS one/two-frame known-motion recovery, long-gap/nonrecursive/cut/ambiguous/semantic/appearance/texture rejection, exact repeat and bounded support')

if __name__=='__main__':main()
