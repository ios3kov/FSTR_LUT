import sys, unittest
from pathlib import Path
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
from reference_correction import interpolate_correction, anchor_pair

class ReferenceCorrectionTests(unittest.TestCase):
 def test_arithmetic_mean_replaces_spike(self):
  actual=interpolate_correction([[9,9,9]], [[.2,.4,.6]], [[.6,.8,1]], .5, [1])
  np.testing.assert_allclose(actual, [[.4,.6,.8]])
 def test_time_weight_and_soft_boundary(self):
  np.testing.assert_allclose(interpolate_correction([[0,0,0]], [[0,0,0]], [[1,1,1]], .25, [.5]), [[.125]*3])
 def test_exclusion_unchanged(self):
  np.testing.assert_array_equal(interpolate_correction([[.1,.2,.3]], [[1]*3], [[1]*3], .5, [0]), np.array([[.1,.2,.3]],np.float32))
 def test_no_cut_long_gap_missing_or_extrapolation(self):
  self.assertEqual(anchor_pair(1,[0,2],{0,1,2}), (0,2,.5))
  for args in ((1,[0,2],{0,1,2},[2]), (2,[0,6],set(range(7)),[]), (1,[0,3],{0,1,3},[]), (0,[1,2],{0,1,2},[])):
   self.assertIsNone(anchor_pair(*args))
 def test_invalid(self):
  with self.assertRaises(ValueError):interpolate_correction([[0]*3], [[0]*3], [[0]*3], .5, [float('nan')])
if __name__=='__main__':unittest.main()
