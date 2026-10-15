"""Analysis tests need the separate research environment; no model required."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import numpy as np
import smart_analysis as s


def main():
    for color in ([0,0,0], [1,1,1], [.4,.4,.4], [.5,.45,.4], [.9,.35,.08], [.01,.02,.06]):
        image = np.full((64,64,3), color, np.float32)
        b, _ = s.estimate_balance(image)
        assert np.array_equal(b, [0,1,1,1]), (color,b)
    ramp = np.linspace(.1,.85,128)[None,:,None]*np.ones((128,1,3))
    assert np.array_equal(s.estimate_balance(ramp)[0], [0,1,1,1])
    # Broad supported slight red cast should be corrected in the right direction.
    cast = s.encode_srgb(s.decode_srgb(ramp)*[1.05,1.,.96]).astype(np.float32)
    b, detail = s.estimate_balance(cast)
    assert detail['wb_applied'] and b[1]<1 and b[3]>1 and np.all((b[1:]>=.95)&(b[1:]<=1.05))
    # Sparse grey patch alone cannot override a strongly colored frame.
    colored = np.full((128,128,3), [.6,.15,.05], np.float32);colored[0:8,0:8]=[.4,.4,.4]
    assert np.array_equal(s.estimate_balance(colored)[0], [0,1,1,1])
    assert s.connected_indices(2,5,[False,True,False,False]) == [2,3,4]
    assert s.connected_indices(0,5,[True,True,False,False]) == [0]
    assert s.cut_between(np.zeros((32,32,3),np.float32),np.ones((32,32,3),np.float32))
    images=[ramp.astype(np.float32)]*5
    masks=[np.full((128,128),p,np.float32) for p in [.2,.4,.6,.8,1.]]
    bset=[np.array([p,1.,1.,1.]) for p in [-.2,-.1,0,.1,.2]]
    forward=[s.stabilize_mask(images,masks,i,[False]*4) for i in range(5)]
    forward_balance=[s.stabilize_balance(bset,i,[False]*4) for i in range(5)]
    for i in [4,0,3,1,2]:
        assert np.array_equal(forward[i],s.stabilize_mask(images,masks,i,[False]*4))
        assert np.array_equal(forward_balance[i],s.stabilize_balance(bset,i,[False]*4))
    assert np.array_equal(s.stabilize_mask(images,masks,2,[True]*4),masks[2])
    assert np.array_equal(s.stabilize_balance(bset,2,[True]*4),bset[2])
    assert np.array_equal(s.stabilize_balance([np.array([.3,1.03,1.,.97]),np.array([0.,1.,1.,1.]),np.array([.3,1.03,1.,.97])],1,[False]*2),[0,1,1,1])
    for invalid in (np.nan,-.5,2.):
        try:
            s.stabilize_mask([images[0]],[np.full((128,128),invalid,np.float32)],0,[])
            raise AssertionError('Invalid mask accepted')
        except ValueError:pass
    zero=[np.zeros((128,128),np.float32)]*5
    zero[1]=zero[3]=np.ones((128,128),np.float32)
    assert not s.stabilize_mask(images,zero,2,[False]*4).any()
    # A spatially localized mask translated with real texture detects reversed
    # flow/warp coordinates; constant masks cannot exercise this behavior.
    rng=np.random.default_rng(721)
    texture=s.cv2.GaussianBlur(rng.uniform(.1,.8,(128,128)).astype(np.float32),(3,3),0)
    textured=np.repeat(texture[...,None],3,-1)
    yy,xx=np.mgrid[:128,:128];patch=(((xx-64)**2+(yy-64)**2)<49).astype(np.float32)
    shift=lambda a,dx:s.cv2.warpAffine(a,np.float32([[1,0,dx],[0,1,0]]),(128,128))
    translated=[shift(textured,-5),textured,shift(textured,5)]
    aligned=s.stabilize_mask(translated,[shift(patch,-5),patch*.5,shift(patch,5)],1,[False]*2)
    assert aligned[64,64]>.6, aligned[64,64]
    assert not aligned[patch==0].any() # No trailing protection outside current support.
    # Photometric rejection and out-of-frame/disocclusion must not add a mask.
    rejected=s.stabilize_mask([np.ones_like(textured),textured],[np.ones_like(patch),patch*.5],1,[False])
    assert np.array_equal(rejected,patch*.5)
    edge_patch=np.zeros_like(patch);edge_patch[:,0:8]=.5
    edge=s.stabilize_mask([shift(textured,20),textured],[np.ones_like(patch),edge_patch],1,[False])
    assert not edge[:,8:].any()
    print('PASS analysis: bounded/uncertain neutral, cast direction, cuts, explicit-window order and mask fallback')


if __name__=='__main__':
    main()
