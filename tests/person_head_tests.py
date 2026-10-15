import unittest,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
from dataclasses import replace
import numpy as np,cv2
from person_head_analysis import PersonEvidence,head_geometry,hand_exclusion,same_person,automatic_window,capacity_status,independent_face_candidates,size_adaptive_region,stabilized_source_height
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
 def test_face_only_association_without_shoulders(self):
  p=replace(self.person(),shoulders=None)
  self.assertIs(same_person(p,[p]),p)
  self.assertIsNone(same_person(p,[p,p]))
  self.assertIsNotNone(same_person(p,[self.person()]))
 def test_independent_face_group(self):
  boxes=[(5,10,25,35),(50,10,70,35),(95,10,115,35)]
  raw={'detail':{'faces':[dict(status='confirmed',detector_score=.95,box=b) for b in boxes]},'mesh':np.ones((128,128),np.float32)}
  found=independent_face_candidates(raw,[],(128,128))
  self.assertEqual(len(found),3)
  for i,(_,support,mask) in enumerate(found):
   for j,b in enumerate(boxes):
    if i!=j:self.assertEqual(mask[int((b[1]+b[3])/2),int((b[0]+b[2])/2)],0)
 def test_independent_face_rejects_shared_mesh(self):
  raw={'detail':{'faces':[dict(status='confirmed',detector_score=.95,box=b) for b in [(40,20,90,80),(50,20,100,80)]]},'mesh':np.ones((128,128),np.float32)}
  self.assertEqual(independent_face_candidates(raw,[],(128,128)),[])
 def test_weak_confirmed_neighbor_cannot_leak(self):
  raw={'detail':{'faces':[dict(status='confirmed',detector_score=score,box=b) for score,b in [(.95,(40,20,90,80)),(.7,(50,20,100,80))]]},'mesh':np.ones((128,128),np.float32)}
  self.assertEqual(independent_face_candidates(raw,[],(128,128)),[])
 def test_independent_face_no_duplicate_body_anchor(self):
  p=self.person();raw={'detail':{'faces':[dict(status='confirmed',detector_score=.95,box=p.anchor.box)]},'mesh':p.anchor.mask}
  self.assertEqual(independent_face_candidates(raw,[p],(128,128)),[])
 def test_face_only_current_protects_without_body(self):
  p=replace(self.person(),shoulders=None)
  m,d=automatic_window(self.images(),[[],[p],[]],1,[False]*2)
  self.assertGreater(m[40:60,50:70].mean(),.3)
  self.assertEqual(m[:5].sum(),0)
 def test_more_than_four_faces(self):
  people=[]
  for x in [10,30,50,70,90,110]:
   mask=np.zeros((128,128),np.float32);mask[30:45,x-4:x+4]=1
   b=(x-4,30,x+4,45);people.append(PersonEvidence(b,(x,37),None,mask,mask,mask*0,FaceAnchor(b,mask)))
  m,_=automatic_window(self.images(),[[],people,[]],1,[False]*2)
  for p in people:self.assertGreater(m[37,int(p.nose[0])],.3)
 def test_light_region_no_face_anchor(self):
  p=replace(self.person(False),shoulders=None,source_height=48.,light_region=True)
  m,d=automatic_window(self.images(),[[],[p],[]],1,[False]*2)
  self.assertGreater(m.sum(),0);self.assertLessEqual(m.max(),.350001)
  self.assertEqual(d[0]['status'],'light_region');self.assertFalse(d[0]['face_geometry_tracking'])
 def test_below_minimum_has_no_spot(self):
  p=replace(self.person(False),source_height=23.,light_region=True)
  m,_=automatic_window(self.images(),[[],[p],[]],1,[False]*2);self.assertEqual(m.sum(),0)
 def test_size_transition_continuous_for_fixed_evidence(self):
  p=self.person();detail=p.skin*.6
  for boundary in (24,32,64,80,128,160,192):
   a=size_adaptive_region(replace(p,source_height=boundary-.001),detail)
   b=size_adaptive_region(replace(p,source_height=boundary+.001),detail)
   self.assertLess(float(np.max(np.abs(a-b))),.001)
 def test_light_region_preserves_hand_veto(self):
  p=replace(self.person(False),source_height=48.,light_region=True,hands=np.ones((128,128),np.float32))
  m,_=automatic_window(self.images(),[[],[p],[]],1,[False]*2);self.assertEqual(m.sum(),0)
 def test_small_face_does_not_invoke_mesh(self):
  from face_skin_analysis import FaceSkinSegmenter
  class Detector:
   def setInputSize(self,size):self.size=size
   def detect(self,image):
    scale=self.size[0]/128;row=np.zeros((1,15),np.float32);row[0,:4]=np.array([40,40,20,24])*scale;row[0,-1]=.95
    return None,row
  obj=object.__new__(FaceSkinSegmenter);obj.detector=Detector()
  # Deliberately no face-landmarker member: any invocation fails the test.
  mask,support,info=obj.analyze_with_support(self.images()[0],64.)
  self.assertEqual(info['faces'][0]['status'],'small_region');self.assertEqual(mask.sum(),0)
 def test_small_region_skips_skin_inference(self):
  from turn_skin_recovery import TurnSkinSegmenter
  obj=object.__new__(TurnSkinSegmenter)
  raw=dict(box=[40,40,60,64],status='small_region',detector_score=.9,scale_agreement=1)
  obj.analyze_with_support=lambda rgb,height:(np.zeros((128,128),np.float32),np.zeros((128,128),np.float32),dict(faces=[raw]))
  # No skin/face models present: this path must not call either.
  result=obj.analyze_turn(self.images()[0],64.)
  self.assertTrue(result['semantic'].any());self.assertTrue(raw['coarse_region_only'])
 def test_coarse_survives_missing_detail_at_transition(self):
  p=replace(self.person(False),shoulders=None,source_height=64.,light_region=True)
  zero=np.zeros((128,128),np.float32)
  a=size_adaptive_region(replace(p,source_height=63.999),zero)
  b=size_adaptive_region(p,zero)
  self.assertGreater(b.sum(),0);self.assertLess(np.max(np.abs(a-b)),.001)
  self.assertGreater(size_adaptive_region(replace(p,source_height=72.),zero).sum(),0)
  self.assertEqual(size_adaptive_region(replace(p,source_height=80.),zero).sum(),0)
 def test_current_anchor_does_not_cut_coarse_holes_at_64(self):
  p=self.person();mask=p.anchor.mask.copy();mask[40:60,50:70]=0
  p=replace(p,source_height=64.,anchor=replace(p.anchor,mask=mask))
  m,_=automatic_window(self.images(),[[],[p],[]],1,[False]*2)
  self.assertGreater(m[50,60],.2)
 def test_full_branch_transition_without_mesh(self):
  from types import SimpleNamespace
  from person_head_analysis import PersonHeadAnalyzer
  analyzer=object.__new__(PersonHeadAnalyzer)
  analyzer._image=lambda rgb:rgb
  analyzer.pose=SimpleNamespace(detect=lambda image:(_ for _ in ()).throw(AssertionError('Small-only frame must skip pose')))
  outputs=[]
  for height,status in [(63.9,'small_region'),(64.1,'no_mesh')]:
   face=dict(box=[40.,20.,80.,20.+height],status=status,detector_score=.95)
   zero=np.zeros((128,128),np.float32)
   analyzer.face=SimpleNamespace(analyze_turn=lambda rgb,minimum_mesh_height:dict(detail=dict(faces=[face]),mesh=zero,semantic=zero))
   # No hand/skin model exists: approximate route cannot call them.
   people,detail=analyzer.analyze(self.images()[0])
   self.assertEqual(len(people),1);self.assertTrue(people[0].light_region)
   m,_=automatic_window(self.images(),[[],people,[]],1,[False]*2);outputs.append(m)
  self.assertGreater(outputs[0].sum(),0)
  self.assertLess(abs(float(outputs[0].sum()/outputs[1].sum())-1),.02)
 def test_dynamic_size_jitter_is_attenuated(self):
  p=replace(self.person(False),source_height=65.,light_region=True,shoulders=None)
  q=replace(p,source_height=63.)
  _,d=automatic_window(self.images(),[[q],[p],[q]],1,[False]*2)
  self.assertEqual(d[0]['effective_height'],64.5)
  self.assertEqual(d[0]['tracked_neighbors'],2)
  _,cut=automatic_window(self.images(),[[q],[p],[q]],1,[True]*2)
  self.assertEqual(cut[0]['effective_height'],65.)
 def test_approach_retreat_size_is_symmetric(self):
  p=self.person()
  for h in range(24,200):
   before=replace(p,source_height=h-1);after=replace(p,source_height=h+1)
   self.assertEqual(stabilized_source_height(replace(p,source_height=h),[before,after]),h)
   self.assertEqual(stabilized_source_height(replace(p,source_height=h),[after,before]),h)
 def test_light_region_render_order_independent(self):
  p=replace(self.person(False),source_height=63.,light_region=True,shoulders=None)
  q=replace(p,source_height=65.)
  images=self.images();people=[[q],[p],[q]]
  a,_=automatic_window(images,people,1,[False]*2)
  automatic_window(images,people,2,[False]*2)
  b,_=automatic_window(images,people,1,[False]*2)
  np.testing.assert_array_equal(a,b)
 def test_coarse_gap_is_bounded_and_cut_safe(self):
  p=replace(self.person(False),source_height=48.,light_region=True,shoulders=None)
  m,d=automatic_window(self.images(),[[p],[],[p]],1,[False]*2)
  self.assertGreater(m.sum(),0);self.assertEqual(d[0]['status'],'coarse_gap')
  cut,_=automatic_window(self.images(),[[p],[],[p]],1,[True,False]);self.assertEqual(cut.sum(),0)
  changed=self.images();changed[1][:]=.9
  rejected,_=automatic_window(changed,[[p],[],[p]],1,[False]*2);self.assertEqual(rejected.sum(),0)
 def test_coarse_gap_competing_people_abstain(self):
  p=replace(self.person(False),source_height=48.,light_region=True,shoulders=None)
  m,_=automatic_window(self.images(),[[p],[],[p,p]],1,[False]*2);self.assertEqual(m.sum(),0)
 def test_coarse_gap_preserves_endpoint_veto(self):
  p=replace(self.person(),source_height=70.,shoulders=None)
  veto=np.ones((128,128),np.float32)
  for q in (replace(p,hands=veto),replace(p,anchor=replace(p.anchor,exclusion=veto))):
   m,_=automatic_window(self.images(),[[q],[],[q]],1,[False]*2);self.assertEqual(m.sum(),0)
 def test_coarse_gap_requires_confirmed_anchor(self):
  p=replace(self.person(),source_height=70.,shoulders=None)
  p=replace(p,anchor=replace(p.anchor,confirmed=False))
  m,_=automatic_window(self.images(),[[p],[],[p]],1,[False]*2);self.assertEqual(m.sum(),0)
 def test_order_repeat(self):
  p=self.person();ims=self.images();people=[[p],[p],[p]]
  a,_=automatic_window(ims,people,1,[False]*2);automatic_window(ims,people,0,[False]*2)
  b,_=automatic_window(ims,people,1,[False]*2);np.testing.assert_array_equal(a,b)

if __name__=='__main__':unittest.main()
