import unittest,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
from dataclasses import replace
import numpy as np,cv2
from person_head_analysis import PersonEvidence,head_geometry,hand_exclusion,same_person,automatic_window,capacity_status
from anchor_face_tracking import FaceAnchor

class PersonTests(unittest.TestCase):
 def person(self,anchor=True):
  one=np.ones((128,128),np.float32);zero=one*0
  a=FaceAnchor((40,20,90,80),one) if anchor else None
  return PersonEvidence((40,20,90,80),(65,50),(30,100,100,100),one,one,zero,a)
 def images(self,n=3):return [np.ones((128,128,3),np.float32)*.5 for _ in range(n)]
 def test_head_confirmation(self):
  p=np.ones((33,4),np.float32);p[:,:2]=[128,80];p[7,:2]=[108,80];p[8,:2]=[148,80];p[11,:2]=[68,170];p[12,:2]=[188,170]
  self.assertIsNotNone(head_geometry(p,(256,256)))
  p[7,2]=.2;self.assertIsNone(head_geometry(p,(256,256)))
 def test_head_invalid(self):
  with self.assertRaises(ValueError):head_geometry(np.zeros((32,4)),(128,128))
 def test_hand_partial_frame(self):
  angle=np.linspace(0,2*np.pi,21,endpoint=False);points=np.c_[2+8*np.cos(angle),50+8*np.sin(angle)]
  m=hand_exclusion([points],(128,128));self.assertGreater(m.sum(),30);self.assertEqual(m[50,2],1)
 def test_unknown_hands_are_not_veto(self):
  self.assertEqual(hand_exclusion([],(128,128)).sum(),0)
 def test_mutual_ambiguity(self):
  p=self.person();self.assertIs(same_person(p,[p]),p);self.assertIsNone(same_person(p,[p,p]))
 def test_ambiguity_not_gap(self):
  p=self.person();target=self.person(False);people=[[p],[p,p],[target],[p],[]]
  m,d=automatic_window(self.images(5),people,2,[False]*4)
  self.assertEqual(m.sum(),0);self.assertEqual(d[0]['reason'],'ambiguous_person_interval')
 def test_global_hand_veto(self):
  p=self.person();v=np.zeros((128,128),np.float32);v[45:55,55:65]=1;q=replace(p,hands=v)
  m,_=automatic_window(self.images(),[[p],[p,q],[p]],1,[False]*2)
  self.assertEqual(m[45:55,55:65].sum(),0);self.assertGreater(m.sum(),0);self.assertEqual(m[:5].sum(),0)
 def test_anchor_exclusion(self):
  p=self.person();v=np.ones((128,128),np.float32);p=replace(p,anchor=replace(p.anchor,exclusion=v))
  m,_=automatic_window(self.images(),[[p],[p],[p]],1,[False]*2);self.assertEqual(m.sum(),0)
 def test_person_evidence_missing(self):
  m,d=automatic_window(self.images(),[[],[],[]],1,[False]*2);self.assertEqual(m.sum(),0);self.assertEqual(d,[])
 def test_invalid_evidence(self):
  p=self.person();bad=p.skin.copy();bad[0,0]=np.nan;p=replace(p,skin=bad)
  with self.assertRaises(ValueError):automatic_window(self.images(),[[p],[p],[p]],1,[False]*2)
 def test_false_current_anchor_checked(self):
  rng=np.random.default_rng(22);rgb=cv2.GaussianBlur(rng.uniform(.2,.8,(128,128,3)).astype(np.float32),(3,3),0)
  changed=rgb.copy();changed[40:60,55:75]=rng.uniform(.35,.65,(20,20,3))
  p=self.person();m,d=automatic_window([rgb,changed,rgb],[[p],[p],[p]],1,[False]*2)
  self.assertTrue(d[0]['two_sided_verified']);self.assertLess(float(m[45:55,60:70].mean()),.05)
 def test_temporal_hand_veto(self):
  rng=np.random.default_rng(31);rgb=cv2.GaussianBlur(rng.uniform(.2,.8,(128,128,3)).astype(np.float32),(3,3),0)
  p=self.person();v=np.zeros((128,128),np.float32);v[40:60,55:75]=1;q=replace(p,hands=v)
  m,_=automatic_window([rgb,rgb,rgb],[[q],[p],[q]],1,[False]*2)
  self.assertLess(float(m[45:55,60:70].max()),.001);self.assertGreater(float(m.sum()),10)
 def test_unchecked_current_is_weaker(self):
  p=self.person();m,d=automatic_window(self.images(),[[p],[p],[p]],1,[True,False])
  self.assertFalse(d[0]['two_sided_verified']);self.assertLessEqual(float(m.max()),.350001)
 def test_soft_hand_cap_is_idempotent(self):
  p=self.person();v=np.zeros((128,128),np.float32);v[45:55,55:65]=.8
  p=replace(p,hands=v,anchor=replace(p.anchor,exclusion=v))
  m,_=automatic_window(self.images(),[[],[p],[]],1,[False]*2)
  # One cap(.2), then unverified strength(.35); final same cap must not square.
  self.assertAlmostEqual(float(m[50,60]),.07,places=6)
 def test_crowd_capacity_report(self):
  self.assertTrue(capacity_status(4)['capacity_reached'])
  self.assertEqual(capacity_status(4)['coverage'],'potentially_truncated')
  self.assertFalse(capacity_status(3)['capacity_reached'])
  with self.assertRaises(ValueError):capacity_status(5)
 def test_crowd_overlap_abstains(self):
  p=self.person();target=self.person(False)
  m,d=automatic_window(self.images(),[[p]*4,[target],[p]*4],1,[False]*2)
  self.assertEqual(m.sum(),0)
 def test_order_repeat(self):
  p=self.person();ims=self.images();people=[[p],[p],[p]]
  a,_=automatic_window(ims,people,1,[False]*2);automatic_window(ims,people,0,[False]*2)
  b,_=automatic_window(ims,people,1,[False]*2);np.testing.assert_array_equal(a,b)

if __name__=='__main__':unittest.main()
