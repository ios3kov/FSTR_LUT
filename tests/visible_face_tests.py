"""Independent input/mask-boundary tests; learned quality needs real images."""
from pathlib import Path
import sys,tempfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import numpy as np
from visible_face_analysis import face_crop,limit_mask,VisibleFaceSegmenter

def main():
    from visible_face_trial import validate_baseline
    baseline=dict(frame_count=156,width=360,height=640,fps=25,analysis_width=720,analysis_height=1280,input_transfer='gamma24')
    validate_baseline(baseline)
    for key,bad in [('fps',24),('analysis_width',360),('analysis_height',640),('input_transfer','srgb')]:
        try:validate_baseline(dict(baseline,**{key:bad}));raise AssertionError('bad baseline accepted')
        except ValueError:pass
    mask=np.array([[0,.3,1],[.8,.2,.9]],np.float32)
    visible=np.array([[1,.49,.5],[0,1,.499]],np.float32)
    assert np.array_equal(limit_mask(mask,visible),np.array([[0,0,1],[0,.2,0]],np.float32))
    assert np.array_equal(limit_mask(mask,np.ones_like(mask)),mask)
    assert not limit_mask(mask,np.zeros_like(mask)).any()
    transform=face_crop([20,20,60,60],(80,80))
    assert np.allclose(transform@np.array([40,40,1]),[128,128])
    assert np.allclose(transform@np.array([10,10,1]),[0,0],atol=1e-5)
    for box in ([20,20,19,60],[-1,20,60,60],[20,20,81,60],[20,np.nan,60,60]):
        try:face_crop(box,(80,80));raise AssertionError('bad box accepted')
        except ValueError:pass
    for bad in (np.ones((3,2)),np.full((2,3),np.nan),np.full((2,3),1.1)):
        try:limit_mask(mask,bad);raise AssertionError('bad mask accepted')
        except ValueError:pass
    with tempfile.TemporaryDirectory() as tmp:
        p=Path(tmp)/'bad.safetensors';p.write_bytes(b'not trusted weights')
        try:VisibleFaceSegmenter(p);raise AssertionError('bad weights accepted')
        except ValueError:pass
    print('PASS threshold boundary, no new coverage, identity/zero, crop coordinates, malformed inputs and weight mismatch')
if __name__=='__main__':main()
