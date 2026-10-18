import sys,unittest
from pathlib import Path
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/benchmark'))
from reference_correction import stable_strengths
class StrengthTests(unittest.TestCase):
 def test_spike_is_removed(self):
  v={i:x for i,x in enumerate([.5,.5,.95,.5,.5])}
  result,used=stable_strengths(v,{i:1 for i in v})
  self.assertEqual(result[2],.5);self.assertEqual(used[2],(0,4,.5))
 def test_gradual_change_retained(self):
  r,_=stable_strengths({i:i/8 for i in range(9)},{i:1 for i in range(9)})
  self.assertAlmostEqual(r[4],.5);self.assertGreater(r[7],r[1])
 def test_low_confidence_missing_cut_no_borrow(self):
  v={0:.2,1:.9,2:.3,4:.8,5:.8};c={i:1 for i in v};c[0]=.1
  r,used=stable_strengths(v,c,[2]);self.assertEqual(r[1],.9);self.assertEqual(used[1],(1,1,0.));self.assertEqual(r[4],.8)
 def test_independent_and_invalid(self):
  self.assertEqual(stable_strengths({0:.2,1:.2},{0:1,1:1})[0],{0:.2,1:.2})
  with self.assertRaises(ValueError):stable_strengths({0:float('nan')},{0:1})
if __name__=='__main__':unittest.main()
