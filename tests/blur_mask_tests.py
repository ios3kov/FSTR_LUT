from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import cv2,numpy as np
from blur_mask_recovery import recover_blur

def main():
    rng=np.random.default_rng(731)
    a=cv2.GaussianBlur(rng.uniform(.2,.8,(96,96,3)).astype(np.float32),(3,3),0)
    support=np.zeros((96,96),np.float32);support[25:70,25:70]=1
    blurred=cv2.GaussianBlur(a,(5,5),0)
    z=np.zeros_like(support)
    imgs=[a,blurred,a];supports=[support]*3;masks=[support,z,support]
    out,info=recover_blur(imgs,supports,masks,1,[False,False])
    assert info['status']=='recovered' and out.sum()>support.sum()*.3
    assert np.all(out<=support) and np.isfinite(out).all()
    repeat,_=recover_blur(imgs,supports,masks,1,[False,False]);assert np.array_equal(out,repeat)
    for cuts in ([True,False],[False,True]):assert not recover_blur(imgs,supports,masks,1,cuts)[0].any()
    assert not recover_blur([a,a,a],supports,masks,1,[False,False])[0].any()
    assert not recover_blur(imgs,supports,[z,z,support],1,[False,False])[0].any()
    occluded=blurred.copy();occluded[38:58,38:58]=[0,.9,0]
    out,meta=recover_blur([a,occluded,a],supports,masks,1,[False,False]);assert meta['status']=='recovered' and out.sum()>support.sum()*.3
    assert not out[40:56,40:56].any()
    # Known two-pixel translation: the target support is independently shifted.
    shift=lambda x,dx:cv2.warpAffine(x,np.array([[1,0,dx],[0,1,0]],np.float32),(96,96))
    moved,meta=recover_blur([a,shift(blurred,2),shift(a,4)],[support,shift(support,2),shift(support,4)],[support,z,shift(support,4)],1,[False,False])
    assert meta['status']=='recovered' and moved.sum()>support.sum()*.25
    assert not moved[:,:27].any() and np.all(moved<=shift(support,2))
    bad=support.copy();bad[0,0]=np.nan
    try:recover_blur(imgs,[support,bad,support],masks,1,[False,False]);raise AssertionError('NaN accepted')
    except ValueError:pass
    try:recover_blur([a,a.astype(np.float64),a],supports,masks,1,[False,False]);raise AssertionError('Unsupported dtype accepted')
    except ValueError:pass
    # An independently textured patch of the same color must not receive
    # near-full protection merely because absolute RGB differences are small.
    from skin_occlusion_stress import run
    for kind in ('flat','textured'):
        diagnostic,_,_=run(recover_blur,731,kind)
        assert diagnostic['interior_mean']<.05 and diagnostic['outside_mean']>.75
    print('PASS blur recovery, unchanged sharp input, cuts, weak seeds, deterministic result, bounds, distinct-color occluder, invalid mask')
if __name__=='__main__':main()
