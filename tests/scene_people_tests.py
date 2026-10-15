import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import numpy as np
from scene_people import detail_mix,face_field,person_field,fuse_people
from scene_analysis import validate_weights
from face_skin_analysis import select_boxes
class Tests(unittest.TestCase):
 def test_bands_continuous(self):
  for kind in ['face','person']:
   for h in [24,32,48,64,80,128,192,256,384,640,896]:
    a,m=detail_mix(h-.001,1,kind);b,n=detail_mix(h+.001,1,kind)
    self.assertLess(abs(a-b),.001);self.assertLess(np.abs(m-n).max(),.001);self.assertAlmostEqual(float(m.sum()),1,6)
 def test_blur_reduces_detail(self):
  for kind in ['face','person']:
   _,m=detail_mix(1200,0,kind);np.testing.assert_array_equal(m,[1,0,0])
 def test_source_scale_equivalence(self):
  np.testing.assert_array_equal(detail_mix(40*4,1)[1],detail_mix(80*2,1)[1])
 def test_no_evidence_exact(self):
  s=np.zeros((5,7,8),np.float32);s[...,6]=.8
  np.testing.assert_array_equal(fuse_people(s,np.zeros((5,7,3),np.float32),np.zeros((5,7),np.float32)),s)
 def test_face_recovers_scene_miss_locally(self):
  s=np.zeros((5,7,8),np.float32);s[...,6]=1;f=np.zeros((5,7),np.float32);f[2,3]=.7
  r=fuse_people(s,np.zeros((5,7,3),np.float32),f);validate_weights(r,(5,7))
  self.assertAlmostEqual(float(r[2,3,0]),.7,6);self.assertAlmostEqual(float(r[2,3,6]),.3,6);np.testing.assert_array_equal(r[0],s[0])
 def test_overlaps_bounded(self):
  s=np.zeros((5,7,8),np.float32);s[...,1]=.9;p=np.zeros((5,7,3),np.float32);p[...,2]=.9
  validate_weights(fuse_people(s,p,np.ones((5,7),np.float32)),(5,7))
 def test_person_not_all_skin(self):
  rgb=np.ones((32,32,3),np.float32)*.3;p=np.zeros((32,32,6),np.float32);p[...,4]=1
  r,_=person_field(rgb,p,[np.ones((32,32),np.float32)],30);self.assertEqual(float(r[...,0].sum()),0);self.assertGreater(float(r[...,2].sum()),0)
 def test_unconfirmed_face_rejected(self):
  rgb=np.ones((32,32,3),np.float32)*.3;m=np.ones((32,32),np.float32)
  r,_=face_field(rgb,m,[dict(box=[4,4,28,28],detector_score=.95,status='no_mesh',scale_agreement=1)],8);self.assertEqual(float(r.sum()),0)
 def test_no_skin_detail_soft_spot(self):
  rgb=np.ones((32,32,3),np.float32)*.3;m=np.zeros((32,32),np.float32)
  r,d=face_field(rgb,m,[dict(box=[4,4,28,28],detector_score=.95,status='confirmed',semantic_mean_inside=0)],8)
  self.assertGreater(r.max(),0);self.assertLessEqual(r.max(),.35);self.assertEqual(float(r[4].sum()),0);self.assertEqual(d[0]['mix'],[1.,0.,0.])
 def test_person_half_probability_continuity(self):
  rgb=np.ones((32,32,3),np.float32)*.3;p=np.zeros((32,32,6),np.float32);p[...,4]=1
  a,_=person_field(rgb,p,[np.full((32,32),.4999,np.float32)],30)
  b,_=person_field(rgb,p,[np.full((32,32),.5001,np.float32)],30)
  self.assertGreater(float(a.sum()),0);self.assertLess(float(np.abs(a-b).max()),.001)
 def test_roundoff_mass_nonnegative(self):
  rng=np.random.default_rng(19)
  for _ in range(20):
   s=rng.random((7,9,8),dtype=np.float32);s/=s.sum(-1,keepdims=True)
   p=rng.random((7,9,3),dtype=np.float32);p/=p.sum(-1,keepdims=True)
   validate_weights(fuse_people(s,p,rng.random((7,9),dtype=np.float32)),(7,9))
 def test_small_source_face_proposal_not_preview_filtered(self):
  proposal=[(np.array([5,5,11,13],float),.95)]
  self.assertEqual(len(select_boxes(proposal,(32,32))),0)
  self.assertEqual(len(select_boxes(proposal,(32,32),minimum_box_size=1)),1)
 def test_geometry_disagreement_not_coarse_skin(self):
  rgb=np.ones((32,32,3),np.float32)*.3;m=np.zeros((32,32),np.float32)
  r,_=face_field(rgb,m,[dict(box=[4,4,28,28],detector_score=.95,status='mesh_disagreement',scale_agreement=2)],8)
  self.assertEqual(float(r.sum()),0)
 def test_mesh_activation_full_branch_continuity(self):
  rgb=np.random.default_rng(7).random((32,32,3),dtype=np.float32);m=np.ones((32,32),np.float32)
  base=dict(box=[4,4,28,28],detector_score=.95,scale_agreement=2,semantic_mean_inside=1)
  a,_=face_field(rgb,m,[dict(base,status='small_region')],63.999/24)
  b,_=face_field(rgb,m,[dict(base,status='confirmed')],64.001/24)
  self.assertLess(float(np.abs(a-b).max()),.001)
  one=dict(base,scale_agreement=1)
  a,_=face_field(rgb,m,[dict(one,status='small_region')],63.999/24)
  b,_=face_field(rgb,m,[dict(one,status='confirmed')],64.001/24)
  self.assertLess(float(np.abs(a-b).max()),.001)
 def test_bad_inputs(self):
  with self.assertRaises(ValueError):detail_mix(float('nan'),1)
  with self.assertRaises(ValueError):detail_mix(32,2)
  with self.assertRaises(ValueError):fuse_people(np.zeros((5,7,8),np.float32),np.ones((5,7,3),np.float32),np.zeros((5,7),np.float32))
if __name__=='__main__':unittest.main()
