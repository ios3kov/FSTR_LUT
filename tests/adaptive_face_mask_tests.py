import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import numpy as np,cv2
from adaptive_face_mask import FaceObservation,spatial_mask,track_window

class AdaptiveTests(unittest.TestCase):
 def obs(self,height=40,shift=0,confidence=1,confirmed=True):
  shape=(240,240);b=(90+shift,100-height/2,150+shift,100+height/2)
  mask=np.zeros(shape,np.float32);mask[max(0,int(b[1])):int(b[3]),int(b[0]):int(b[2])]=1
  detail=mask.copy();detail[90:100,110:130]=0
  return FaceObservation(b,confidence,confirmed,mask,detail,simple_allowed=True)
 def frames(self):
  rng=np.random.default_rng(18);image=cv2.GaussianBlur(rng.random((240,240,3),dtype=np.float32),(3,3),0)
  return [image.copy() for _ in range(5)]
 def test_small_cap_and_tiny(self):
  self.assertEqual(spatial_mask(self.obs(23),(240,240)).sum(),0)
  m=spatial_mask(self.obs(40),(240,240));self.assertGreater(m.sum(),0);self.assertLessEqual(m.max(),.35)
 def test_detail_and_confirmation(self):
  o=self.obs(200);m=spatial_mask(o,(240,240));self.assertEqual(m[95,120],0)
  self.assertEqual(spatial_mask(self.obs(40,confirmed=False),(240,240)).sum(),0)
 def test_continuity(self):
  for h in (24,32,48,64,80,128,160,192):
   # Supplied masks change discretely at raster boundaries; assess simple blend
   # weights with fixed observations/masks and only source-size changing.
   o=self.obs(40);a=spatial_mask(o,(240,240),(h-.001)/40);b=spatial_mask(o,(240,240),(h+.001)/40)
   self.assertLess(float(np.abs(a-b).max()),.001)
 def test_preview_scale_policy(self):
  a=self.obs(40);b=self.obs(80)
  # Center values prove same native-height policy, independent of raster shape.
  self.assertAlmostEqual(float(spatial_mask(a,(240,240),2)[105,120]),float(spatial_mask(b,(240,240),1)[105,120]),places=6)
 def test_two_sided_and_order(self):
  ims=self.frames();o=self.obs();obs=[[o],[o],[],[o],[o]]
  a,info=track_window(ims,obs,2,[False]*5,gap_evidence=np.ones((240,240),np.float32));self.assertGreater(a.sum(),0);self.assertEqual(info[0]['status'],'two_sided_gap')
  track_window(ims,obs,0,[False]*5)
  b,_=track_window(ims,obs,2,[False]*5,gap_evidence=np.ones((240,240),np.float32));np.testing.assert_array_equal(a,b)
 def test_cut_and_ambiguity(self):
  ims=self.frames();o=self.obs();obs=[[o],[o],[],[o],[o]]
  m,_=track_window(ims,obs,2,[False,False,True,False,False],gap_evidence=np.ones((240,240),np.float32));self.assertEqual(m.sum(),0)
  obs[1]=[o,self.obs(40,shift=4)];obs[0]=obs[1]
  m,_=track_window(ims,obs,2,[False]*5,gap_evidence=np.ones((240,240),np.float32));self.assertEqual(m.sum(),0)
 def test_occlusion_and_no_texture(self):
  ims=self.frames();o=self.obs();obs=[[o],[o],[],[o],[o]]
  ims[2][:]=0
  m,_=track_window(ims,obs,2,[False]*5,gap_evidence=np.ones((240,240),np.float32));self.assertEqual(m.sum(),0)
  ims=[np.ones((240,240,3),np.float32)*.5 for _ in range(5)]
  m,_=track_window(ims,obs,2,[False]*5,gap_evidence=np.ones((240,240),np.float32));self.assertEqual(m.sum(),0)
 def test_missing_current_evidence(self):
  ims=self.frames();o=self.obs();obs=[[o],[o],[],[o],[o]]
  m,_=track_window(ims,obs,2,[False]*5);self.assertEqual(m.sum(),0)
 def test_invalid_flow_preserves_current(self):
  ims=[np.ones((240,240,3),np.float32)*.5 for _ in range(3)];o=self.obs()
  m,_=track_window(ims,[[o],[o],[o]],1,[False]*3)
  np.testing.assert_array_equal(m,spatial_mask(o,(240,240)))
 def test_competing_unconfirmed(self):
  ims=self.frames();o=self.obs();weak=self.obs(40,shift=2,confidence=.7,confirmed=False)
  m,info=track_window(ims,[[o],[o,weak],[o],[o],[o]],2,[False]*5)
  self.assertEqual(info[0]['neighbors'],3)
 def test_translation(self):
  im=self.frames()[0];ims=[];obs=[]
  for shift in (-2,0,2):
   ims.append(cv2.warpAffine(im,np.array([[1,0,shift],[0,1,0]],np.float32),(240,240)))
   obs.append([self.obs(40,shift=shift)] if shift else [])
  m,info=track_window(ims,obs,1,[False]*3,gap_evidence=np.ones((240,240),np.float32))
  self.assertGreater(float(m.sum()),1);self.assertEqual(info[0]['status'],'two_sided_gap')
  ys,xs=np.mgrid[:240,:240];self.assertLess(abs(float((m*xs).sum()/m.sum())-120),2)
 def test_two_frame_gap_and_right_cut(self):
  ims=self.frames();o=self.obs();obs=[[o],[],[],[o],[o]]
  m,_=track_window(ims,obs,1,[False]*5,gap_evidence=np.ones((240,240),np.float32));self.assertGreater(m.sum(),0)
  m,_=track_window(ims,obs,1,[False,False,True,False,False],gap_evidence=np.ones((240,240),np.float32));self.assertEqual(m.sum(),0)
 def test_gap_evidence_bounds(self):
  ims=self.frames();o=self.obs();obs=[[o],[o],[],[o],[o]];e=np.zeros((240,240),np.float32);e[:,120:]=.1
  m,_=track_window(ims,obs,2,[False]*5,gap_evidence=e)
  self.assertEqual(float(m[:,:120].sum()),0);self.assertLessEqual(float(m.max()),.050001)
  m,info=track_window(ims,obs,2,[False]*5,gap_evidence=e*0);self.assertEqual(m.sum(),0);self.assertEqual(info,[])
 def test_simple_explicit_permission(self):
  o=self.obs();z=np.zeros((240,240),np.float32)
  o=FaceObservation(o.box,1,True,z,z,detail_confidence=0)
  self.assertEqual(spatial_mask(o,(240,240)).sum(),0)
 def test_bounds(self):
  with self.assertRaises(ValueError):track_window(self.frames()*2,[[]]*10,2,[False]*10)
  with self.assertRaises(ValueError):spatial_mask(self.obs(),(240,240),0)

if __name__=='__main__':unittest.main()
