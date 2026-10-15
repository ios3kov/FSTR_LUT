import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import numpy as np
from video_anchor_analysis import choose_anchor,warp_anchor,cut_between,combine_prediction,combine_geometry
class Tests(unittest.TestCase):
 def setUp(self):
  self.im=np.random.default_rng(3).random((48,48,3),dtype=np.float32);self.base=np.zeros((48,48,8),np.float32);self.base[...,6]=.8
 def test_absent_anchor(self):
  with self.assertRaises(ValueError):choose_anchor([self.im],[self.base],[0])
 def test_anchor_automatic_repeat(self):
  m=self.base.copy();m[8:24,8:24,0]=.7;m[8:24,8:24,6]=.2
  a,mask=choose_anchor([self.im,self.im],[m,m],[1,0]);self.assertEqual(a['index'],0);self.assertEqual(int(mask.sum()),256)
 def test_identity_warp(self):
  m=np.zeros((48,48),np.float32);m[12:35,12:35]=1;p,t=warp_anchor(self.im,self.im,m)
  self.assertLess(float(np.abs(p-m).max()),.04);self.assertGreater(float(t[15:30,15:30].mean()),.99)
 def test_cut(self):self.assertTrue(cut_between([self.im*0,self.im*0+1],0,1))
 def test_camera_reposition_same_palette_not_cut(self):
  self.assertFalse(cut_between([self.im,np.roll(self.im,20,axis=0)],0,1))
 def test_disagreement_no_override(self):
  z=np.zeros((48,48),np.float32);o=z+1;r,w=combine_prediction(self.base,z,o,o,o,.8);np.testing.assert_array_equal(r,self.base);self.assertEqual(float(w.sum()),0)
 def test_no_person_no_override(self):
  z=np.zeros((48,48),np.float32);o=z+1;r,w=combine_prediction(self.base,o,o,o,z,.8);np.testing.assert_array_equal(r,self.base)
 def test_no_trust_no_override(self):
  z=np.zeros((48,48),np.float32);o=z+1;r,w=combine_prediction(self.base,o,o,z,o,.8);np.testing.assert_array_equal(r,self.base)
 def test_clothing_veto(self):
  b=self.base*0;b[...,2]=1;o=np.ones((48,48),np.float32);r,w=combine_prediction(b,o,o,o,o,.8);np.testing.assert_array_equal(r,b)
 def test_bounded_replacement(self):
  o=np.ones((48,48),np.float32);r,w=combine_prediction(self.base,o,o,o,o,.8);self.assertGreater(float(r[...,0].mean()),.79);self.assertLessEqual(float(r.sum(-1).max()),1.000001)
 def test_weak_person_support_does_not_dim_confident_track(self):
  o=np.ones((48,48),np.float32);r,w=combine_prediction(self.base,o,o,o,o*.3,.8);self.assertGreater(float(r[...,0].mean()),.79)
 def test_negligible_person_support_abstains(self):
  o=np.ones((48,48),np.float32);r,w=combine_prediction(self.base,o,o,o,o*.09,.8);np.testing.assert_array_equal(r,self.base)
 def test_hybrid_untrusted_flow_is_exact_sam(self):
  o=np.ones((48,48),np.float32);z=o*0;f,b,w=combine_geometry(o,o,z,z,z);np.testing.assert_array_equal(f,o);np.testing.assert_array_equal(b,o)
 def test_hybrid_conflicting_flow_is_ignored(self):
  o=np.ones((48,48),np.float32);z=o*0;f,b,w=combine_geometry(o,o,z,z,o);np.testing.assert_array_equal(f,o);self.assertEqual(float(w.max()),0)
 def test_hybrid_agreeing_flow_bounded_refinement(self):
  o=np.ones((48,48),np.float32);f,b,w=combine_geometry(o*.8,o*.8,o*.85,o*.85,o);self.assertTrue(np.all(f>=.8));self.assertTrue(np.all(f<=.85));self.assertLessEqual(float(w.max()),.25)
 def test_background_agreement_is_not_object_evidence(self):
  z=np.zeros((48,48),np.float32);f,b,w=combine_geometry(z,z,z,z,z+1);self.assertEqual(float(w.sum()),0)
 def test_invalid(self):
  o=np.ones((48,48),np.float32)
  with self.assertRaises(ValueError):combine_prediction(self.base,o,o,o,o,float('nan'))
if __name__=='__main__':unittest.main()
