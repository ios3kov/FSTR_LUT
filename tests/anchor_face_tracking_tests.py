import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import numpy as np,cv2
from anchor_face_tracking import FaceAnchor,transfer_blurred_face,track_anchor,coarse_weight

class AnchorTests(unittest.TestCase):
 def fixture(self):
  rng=np.random.default_rng(731);a=cv2.GaussianBlur(rng.uniform(.2,.8,(128,128,3)).astype(np.float32),(3,3),0)
  mask=np.zeros((128,128),np.float32);mask[30:95,30:95]=1;mask[48:55,45:75]=0
  self.shift=lambda x,d:cv2.warpAffine(x,np.array([[1,0,d],[0,1,0]],np.float32),(128,128))
  images=[a,self.shift(cv2.GaussianBlur(a,(5,5),0),2),self.shift(a,4)]
  anchors=[[FaceAnchor((30,30,95,95),mask)],[],[FaceAnchor((34,30,99,95),self.shift(mask,4))]]
  return images,anchors,np.ones((128,128),np.float32),np.zeros((128,128),np.float32)
 def test_motion_and_blur(self):
  ims,a,e,v=self.fixture();m,d=transfer_blurred_face(ims,a,1,[False]*2,e,v)
  self.assertEqual(d['status'],'transferred');self.assertGreater(m.sum(),100);self.assertGreater(d['blur_weight'],0)
  self.assertLess(abs(d['tracking'][0]['motion'][0]-2),.5);self.assertLessEqual(m.max(),1)
 def test_cuts_ambiguity_and_missing(self):
  ims,a,e,v=self.fixture()
  for cuts in ([True,False],[False,True]):self.assertEqual(transfer_blurred_face(ims,a,1,cuts,e,v)[0].sum(),0)
  a[0]*=2;self.assertEqual(transfer_blurred_face(ims,a,1,[False]*2,e,v)[0].sum(),0)
  a[0]=[];self.assertEqual(transfer_blurred_face(ims,a,1,[False]*2,e,v)[0].sum(),0)
 def test_current_veto(self):
  ims,a,e,v=self.fixture();v[45:75,45:75]=1
  m,_=transfer_blurred_face(ims,a,1,[False]*2,e,v);self.assertGreater(m.sum(),100);self.assertEqual(m[45:75,45:75].sum(),0)
  m,_=transfer_blurred_face(ims,a,1,[False]*2,e*0,v);self.assertEqual(m.sum(),0)
 def test_not_blur_and_uniform(self):
  ims,a,e,v=self.fixture();ims[1]=self.shift(ims[0],2)
  m,d=transfer_blurred_face(ims,a,1,[False]*2,e,v)
  self.assertEqual(d['status'],'transferred');self.assertTrue(d['detail_uncertain']);self.assertLessEqual(float(m.max()),.350001)
  ims=[np.full_like(ims[0],.5)]*3
  self.assertEqual(transfer_blurred_face(ims,a,1,[False]*2,e,v)[0].sum(),0)
 def test_similar_color_occluder(self):
  ims,a,e,v=self.fixture();rng=np.random.default_rng(97)
  ims[1][48:78,48:78]=rng.uniform(.35,.65,(30,30,3))
  m,_=transfer_blurred_face(ims,a,1,[False]*2,e,v)
  self.assertLess(float(m[54:72,54:72].mean()),.05)
 def test_repeat_and_two_frame_gap(self):
  ims,a,e,v=self.fixture();m,_=transfer_blurred_face(ims,a,1,[False]*2,e,v)
  again,_=transfer_blurred_face(ims,a,1,[False]*2,e,v);np.testing.assert_array_equal(m,again)
  # Explicit four-frame window with a second missing observation.
  images=[ims[0],ims[1],ims[1],ims[2]];anchors=[a[0],[],[],a[2]]
  m,d=transfer_blurred_face(images,anchors,1,[False]*3,e,v);self.assertEqual(d['status'],'transferred')
 def test_anchor_exclusion_survives_simplification(self):
  ims,a,e,v=self.fixture();excluded=np.zeros_like(e);excluded[45:75,45:75]=1
  a[0][0]=FaceAnchor(a[0][0].box,a[0][0].mask,exclusion=excluded)
  a[2][0]=FaceAnchor(a[2][0].box,a[2][0].mask,exclusion=self.shift(excluded,4))
  m,d=transfer_blurred_face(ims,a,1,[False]*2,e,v)
  self.assertEqual(d['status'],'transferred');self.assertEqual(m[48:72,50:74].sum(),0)
 def test_detail_weight_continuity(self):
  for x in (.15,.55,.75,.85):
   self.assertLess(abs(coarse_weight(x-1e-5)-coarse_weight(x+1e-5)),.001)
  self.assertTrue(all(0<=coarse_weight(x)<=1 for x in np.linspace(0,4,100)))
 def test_invalid(self):
  ims,a,e,v=self.fixture();e[0,0]=np.nan
  with self.assertRaises(ValueError):transfer_blurred_face(ims,a,1,[False]*2,e,v)

if __name__=='__main__':unittest.main()
