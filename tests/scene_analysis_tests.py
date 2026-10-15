import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
import numpy as np
from scene_analysis import refine_weights,stabilize_window,validate_weights,information_support
class Tests(unittest.TestCase):
 def setUp(self):
  self.im=np.full((32,32,3),.3,np.float32);self.m=np.zeros((32,32,8),np.float32);self.m[...,3]=.8
 def test_uniform_abstention(self):
  self.assertEqual(information_support(self.im),0)
  rgb=self.im.copy();rgb[:16]=.8;self.assertEqual(information_support(rgb),1)
 def test_spatial_mass(self):
  m=self.m.copy();m[:16]=0;r=refine_weights(self.im,m);validate_weights(r,(32,32));self.assertTrue(0<r[15,16,3]<.8)
 def test_no_support(self):
  z=self.m*0;self.assertTrue(np.array_equal(stabilize_window({0:(self.im,z)}),z))
 def test_order(self):
  f={i:(self.im,self.m) for i in range(-2,3)};self.assertTrue(np.array_equal(stabilize_window(f),stabilize_window(dict(reversed(list(f.items()))))))
 def test_cut(self):
  z=self.m*0;r=stabilize_window({0:(self.im,z),-1:(self.im*0,self.m),1:(self.im*0,self.m)});self.assertEqual(float(r.sum()),0)
 def test_short_not_long(self):
  z=self.m*0;short={-1:(self.im,self.m),0:(self.im,z),1:(self.im,self.m)};self.assertGreater(stabilize_window(short).sum(),0)
  long={-2:(self.im,self.m),-1:(self.im,z),0:(self.im,z),1:(self.im,z),2:(self.im,self.m)};self.assertEqual(float(stabilize_window(long).sum()),0)
 def test_weak_one_sided(self):
  self.assertEqual(float(stabilize_window({0:(self.im,self.m*0),1:(self.im,self.m*.05)}).sum()),0)
 def test_bad_neighbor_geometry(self):
  with self.assertRaises(ValueError):stabilize_window({0:(self.im,self.m),1:(self.im[:16],self.m)})
 def test_one_sided(self):
  self.assertEqual(float(stabilize_window({0:(self.im,self.m*0),1:(self.im,self.m)}).sum()),0)
if __name__=='__main__':unittest.main()
