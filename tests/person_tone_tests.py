import sys,unittest
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
from person_tone_analysis import associate,ownership,observation,attach_body_skin,stabilize_fields,two_sided_confidence,mean_face_fields

def obs(x,color=(1.5,.9,.5)):
 return dict(box=np.array([x,0,x+20,20],float),descriptor=np.array(color))
class PersonToneTests(unittest.TestCase):
 def test_approach_and_reorder(self):
  r=associate([[obs(0),obs(100,(1.1,1.,.8))],[obs(98,(1.1,1.,.8)),obs(2)]])
  self.assertEqual(r,[[0,1],[1,0]])
 def test_ambiguity_does_not_borrow(self):
  r=associate([[obs(0),obs(40)],[obs(19),obs(21)]])
  self.assertTrue(set(r[0]).isdisjoint(r[1]))
 def test_ambiguous_interval_ends_identity(self):
  r=associate([[obs(0),obs(40)],[obs(19),obs(21)],[obs(0),obs(40)]])
  self.assertEqual(r[1],[None,None]);self.assertTrue(set(r[0]).isdisjoint(r[2]))
 def test_disjoint_person_does_not_change_edge(self):
  a=np.array([[[.1,.2,0.]]],np.float32);b=np.concatenate([a,np.array([[[0.,0.,.7]]],np.float32)])
  np.testing.assert_array_equal(ownership(a)[0,:,:2],ownership(b)[0,:,:2])
 def test_gap_and_cut(self):
  self.assertEqual(associate([[obs(0)],[],[obs(1)]])[-1],[0])
  self.assertEqual(associate([[obs(0)],[],[],[obs(1)]])[-1],[1])
  self.assertEqual(associate([[obs(0)],[obs(1)]],cut_before=[1])[-1],[1])
 def test_color_jump(self):
  self.assertEqual(associate([[obs(0)],[obs(0,(.1,1.8,2.))]])[-1],[1])
 def test_ownership(self):
  a=np.array([[[.8,.7,.1]],[[.8,.1,.7]]],np.float32);b=ownership(a)
  self.assertEqual(float(b[:,0,0].sum()),0)
  self.assertTrue((b.sum(0)<=1).all())
  np.testing.assert_allclose(ownership(a[::-1])[::-1],b)
 def test_color_alone_not_confirmation(self):
  self.assertIsNone(observation([0,0,10,10],np.zeros((10,10)),np.full((10,10,3),.3)))
 def test_body_needs_face_and_material(self):
  rgb=np.full((10,10,3),.2,np.float32);mask=np.zeros((10,10),np.float32);mask[:3,:3]=1
  o=observation([0,0,3,3],mask,rgb);parts=np.zeros((10,10,6),np.float32);parts[...,0]=1
  poses=np.ones((1,10,10),np.float32)
  np.testing.assert_array_equal(attach_body_skin([o],poses,parts,rgb)[0]['mask'],mask)
  parts[6:,6:,0]=0;parts[6:,6:,2]=1
  self.assertEqual(float(attach_body_skin([o],poses,parts,rgb)[0]['mask'][8,8]),1.)
  other=dict(o,box=np.array([6,0,9,3]),mask=np.roll(mask,6,axis=1))
  result=attach_body_skin([o,other],poses,parts,rgb)
  self.assertEqual(float(result[0]['mask'][8,8]),0.)
  self.assertEqual(attach_body_skin([],poses,parts,rgb),[])
 def test_only_best_pose_can_attach_to_face(self):
  rgb=np.full((10,10,3),.2,np.float32);mask=np.zeros((10,10),np.float32);mask[:3,:3]=1
  o=observation([0,0,3,3],mask,rgb);parts=np.zeros((10,10,6),np.float32);parts[...,0]=1;parts[6:,6:,0]=0;parts[6:,6:,2]=1
  first=np.zeros((10,10),np.float32);first[:3,:3]=1
  second=np.ones((10,10),np.float32);second[:3,:3]=.6
  result=attach_body_skin([o],np.stack([first,second]),parts,rgb)
  self.assertEqual(float(result[0]['mask'][8,8]),0.)
 def test_temporal_cannot_restore_current_veto(self):
  rgb=np.full((24,24,3),.3,np.float32);full=np.full((24,24),.8,np.float32);middle=full.copy();middle[10:14,10:14]=0
  frames=[[dict(box=np.array([0,0,24,24]),mask=m,descriptor=np.ones(3))] for m in (full,middle,full)]
  result=stabilize_fields([rgb]*3,frames,[[0],[0],[0]])
  self.assertTrue((result[1][0]['mask'][10:14,10:14]==0).all())
  self.assertTrue((result[1][0]['mask']<=middle).all())
 def test_gradual_approach_preserves_track(self):
  frames=[]
  for size in (20,24,29,35,42,50,60,72,86,103,124,149,179,215,258,310):
   frames.append([dict(box=np.array([200-size/2,200-size/2,200+size/2,200+size/2]),descriptor=np.array([1.5,.9,.5]))])
  self.assertTrue(all(v==[0] for v in associate(frames)))
 def test_temporal_cut_and_missing_person(self):
  rgb=np.full((24,24,3),.3,np.float32);left=np.full((24,24),.8,np.float32);right=np.full((24,24),.2,np.float32)
  observations=[[dict(box=np.array([0,0,24,24]),mask=m,descriptor=np.ones(3))] for m in (left,right)]
  result=stabilize_fields([rgb,rgb],observations,[[0],[1]],[1]);np.testing.assert_array_equal(result[0][0]['mask'],left)
  self.assertEqual(stabilize_fields([rgb],[[]],[[]]),[[]])
 def test_unknown_is_recovered_but_known_exclusion_is_not(self):
  rgb=np.full((24,24,3),.3,np.float32);full=np.full((24,24),.8,np.float32);middle=full.copy();middle[10:14,10:14]=0
  ceiling=np.ones((24,24),np.float32);ceiling[10:12,10:12]=0
  frames=[[dict(box=np.array([0,0,24,24]),mask=m,ceiling=ceiling,descriptor=np.ones(3))] for m in (full,middle,full)]
  result=stabilize_fields([rgb]*3,frames,[[0],[0],[0]])[1][0]['mask']
  self.assertTrue((result[10:12,10:12]==0).all());self.assertGreater(float(result[13,13]),.75)
 def test_unknown_face_still_has_independent_geometry(self):
  rgb=np.full((10,10,3),.2,np.float32);raw=np.zeros((10,10),np.float32);geometry=np.ones((10,10),np.float32)
  self.assertIsNotNone(observation([0,0,10,10],raw,rgb,descriptor_support=geometry))
  self.assertIsNone(observation([0,0,10,10],raw,rgb))
 def test_one_sided_cannot_invent_skin(self):
  rgb=np.full((24,24,3),.3,np.float32);full=np.full((24,24),.8,np.float32);empty=np.zeros((24,24),np.float32)
  frames=[[dict(box=np.array([0,0,24,24]),mask=m,ceiling=np.ones_like(m),descriptor=np.ones(3))] for m in (full,empty)]
  self.assertEqual(float(stabilize_fields([rgb]*2,frames,[[0],[0]])[1][0]['mask'].max()),0.)
 def test_three_frame_unknown_gap_does_not_bridge(self):
  rgb=np.full((24,24,3),.3,np.float32);full=np.full((24,24,8),.0,np.float32);full[...,0]=.8;empty=full*0
  window={i:(rgb,full if abs(i)==2 else empty) for i in (-2,-1,0,1,2)}
  self.assertEqual(float(two_sided_confidence(window).max()),0.)
 def test_person_mean_ignores_unknown_and_preserves_exclusion(self):
  geometry=np.ones((24,24),np.float32);ceiling=geometry.copy();ceiling[10:12,10:12]=0
  frames=[[dict(box=np.array([0,0,24,24]),mask=np.full((24,24),v,np.float32),face_geometry=geometry,face_ceiling=ceiling)] for v in (.7,0.,.9)]
  result=mean_face_fields(frames,frames,[[0],[0],[0]])
  self.assertTrue(all(abs(float(row[0]['mask'][15,15])-.8)<1e-5 for row in result))
  self.assertTrue(all(float(row[0]['mask'][10,10])==0 for row in result))
  split=mean_face_fields(frames,frames,[[0],[1],[2]])
  self.assertGreater(float(split[2][0]['mask'][15,15]),float(split[0][0]['mask'][15,15]))
 def test_probability_blend_tolerates_only_roundoff(self):
  from person_sam_fields import blend_face_field
  shape=(4,4);v=blend_face_field(np.full(shape,.8,np.float32),np.zeros(shape,np.float32),np.full(shape,1+1e-7,np.float32),np.zeros(shape,np.float32))
  self.assertEqual(float(v.min()),0.)
  with self.assertRaises(ValueError):blend_face_field(v,v,np.full(shape,1.1),v)
 def test_repeat(self):
  frames=[[obs(i),obs(100-i,(1.1,1.,.8))] for i in range(8)]
  self.assertEqual(associate(frames),associate(frames))
if __name__=='__main__':unittest.main()
