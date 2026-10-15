import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
from dataclasses import replace
import cv2,numpy as np
from person_head_analysis import PersonEvidence
from anchor_face_tracking import FaceAnchor
from color_region_tracking import color_region_window,detail_confidence

class ColorRegionTests(unittest.TestCase):
 def person(self,anchor=False):
  zero=np.zeros((128,128),np.float32);mask=zero.copy();mask[25:105,40:90]=1
  return PersonEvidence((40,25,90,105),(65,65),None,mask,zero,zero,FaceAnchor((40,25,90,105),mask) if anchor else None,240.,True)
 def images(self):return [np.ones((128,128,3),np.float32)*.5 for _ in range(3)]
 def test_large_blurred_region_has_no_holes(self):
  p=self.person();m,d=color_region_window(self.images(),[[p]]*3,1,[False]*2)
  self.assertGreater(float(m[50:80,55:75].min()),.5);self.assertEqual(d[0]['detail_weight'],0)
 def test_same_color_hand_is_not_veto(self):
  p=self.person();q=replace(p,hands=np.ones((128,128),np.float32))
  a,_=color_region_window(self.images(),[[p]]*3,1,[False]*2)
  b,_=color_region_window(self.images(),[[q]]*3,1,[False]*2)
  np.testing.assert_array_equal(a,b)
 def test_background_not_globally_selected(self):
  m,_=color_region_window(self.images(),[[]]*3,1,[False]*2);self.assertEqual(m.sum(),0)
 def test_details_cannot_puncture_base(self):
  p=self.person(True);mask=p.anchor.mask.copy();mask[55:75,55:75]=0;p=replace(p,anchor=replace(p.anchor,mask=mask))
  rng=np.random.default_rng(4);rgb=rng.uniform(0,1,(128,128,3)).astype(np.float32)
  m,_=color_region_window([rgb]*3,[[p]]*3,1,[False]*2);self.assertGreater(m[65,65],.3)
 def test_large_gap_cut_and_appearance(self):
  p=self.person(True);m,d=color_region_window(self.images(),[[p],[],[p]],1,[False]*2)
  self.assertGreater(m.sum(),0);self.assertEqual(d[0]['status'],'coarse_gap')
  cut,_=color_region_window(self.images(),[[p],[],[p]],1,[True,False]);self.assertEqual(cut.sum(),0)
  rgb=self.images();rgb[1][:]=.9;changed,_=color_region_window(rgb,[[p],[],[p]],1,[False]*2);self.assertEqual(changed.sum(),0)
 def test_blurring_reduces_detail_weight(self):
  rng=np.random.default_rng(6);rgb=rng.uniform(.1,.9,(128,128,3)).astype(np.float32);p=self.person(True)
  self.assertGreater(detail_confidence(rgb,p),detail_confidence(cv2.GaussianBlur(rgb,(0,0),4.),p))
 def test_neighbors_cannot_restore_details_to_blur(self):
  p=self.person(True);rng=np.random.default_rng(8);sharp=rng.uniform(.1,.9,(128,128,3)).astype(np.float32)
  blurred=np.ones_like(sharp)*.5
  _,d=color_region_window([sharp,blurred,sharp],[[p]]*3,1,[False]*2)
  self.assertEqual(d[0]['detail_weight'],0.)
 def test_repeat_and_reverse(self):
  p=self.person();images=self.images();people=[[p]]*3
  a,_=color_region_window(images,people,1,[False]*2);color_region_window(images,people,2,[False]*2)
  b,_=color_region_window(images,people,1,[False]*2);np.testing.assert_array_equal(a,b)
 def test_bounds_and_invalid_input(self):
  p=self.person();m,_=color_region_window(self.images(),[[p]]*3,1,[False]*2)
  self.assertTrue(np.isfinite(m).all());self.assertLessEqual(m.max(),.650001);self.assertEqual(m[:5].sum(),0)
  with self.assertRaises(ValueError):color_region_window(self.images(),[[replace(p,source_height=float('nan'))]]*3,1,[False]*2)
 def test_invalid_association_geometry(self):
  p=replace(self.person(),nose=(float('nan'),65))
  with self.assertRaises(ValueError):color_region_window(self.images(),[[p]]*3,1,[False]*2)
 def test_unknown_anchor_not_detail(self):
  p=self.person(True);p=replace(p,anchor=replace(p.anchor,confirmed=False))
  _,d=color_region_window(self.images(),[[p]]*3,1,[False]*2);self.assertEqual(d[0]['detail_weight'],0)

if __name__=='__main__':unittest.main()
