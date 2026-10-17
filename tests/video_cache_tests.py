"""Exercise refusal paths at the real comparison CLI, before native rendering."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

RUNNER = Path(__file__).resolve().parents[1] / 'tools/benchmark/video_anchor_compare.py'


class CacheTests(unittest.TestCase):
    def reject(self, defect, expected):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            manifest = dict(cases=[{}], anchors=[dict(direction=d,index=0) for d in ('forward','backward')])
            path = root/'manifest.json'
            path.write_text(json.dumps(manifest))
            folder = root/'flow-final'
            folder.mkdir()
            hashes = {}
            for direction in ('forward','backward'):
                for suffix in ('','-trust'):
                    name = direction+'-000'+suffix+'.npy'
                    (folder/name).write_bytes(b'fixture')
                    hashes[name] = hashlib.sha256(b'fixture').hexdigest()
            record = dict(manifest_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),output_sha256=hashes)
            if defect == 'manifest':
                record['manifest_sha256'] = 'wrong'
            elif defect == 'missing':
                del hashes['forward-000.npy']
            elif defect == 'changed':
                (folder/'forward-000.npy').write_bytes(b'changed')
            (folder/'evidence.json').write_text(json.dumps(record))
            run = subprocess.run([sys.executable,str(RUNNER),str(root),'unused-baseline','unused-renderer','unused-lut'],capture_output=True,text=True)
            self.assertNotEqual(run.returncode,0)
            self.assertIn(expected,run.stderr)
            self.assertFalse(list((root/'comparison-final').glob('*.rgba32f')))

    def test_empty_repeat_is_not_success(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            record={key:None for key in ('manifest_sha256','model','source_sha256','seed','deterministic_algorithms','loaded_state_sha256','mps_environment','torch','device','runner_sha256')}
            record.update(output_sha256={},runs=[])
            for name in ('a','b'):
                (root/name).mkdir();(root/name/'evidence.json').write_text(json.dumps(record))
            run=subprocess.run([sys.executable,str(RUNNER.with_name('video_repeat_report.py')),str(root/'a'),str(root/'b'),str(root/'report.json')],capture_output=True,text=True)
            self.assertNotEqual(run.returncode,0);self.assertIn('Empty or incomplete prediction inventory',run.stderr);self.assertFalse((root/'report.json').exists())

    def test_report_refuses_other_roi_cache(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'comparison-reviewed').mkdir();(root/'sam2-final').mkdir()
            (root/'comparison-reviewed/evidence.json').write_text(json.dumps(dict(prediction_evidence_sha256={'sam2-final':'wrong'})))
            (root/'sam2-final/evidence.json').write_text('{}')
            run=subprocess.run([sys.executable,str(RUNNER.with_name('video_anchor_report.py')),str(root)],capture_output=True,text=True)
            self.assertNotEqual(run.returncode,0);self.assertIn('ROI cache does not match rendered comparison',run.stderr)

    def test_wrong_input_manifest(self):
        self.reject('manifest','Prediction belongs to different inputs')

    def test_incomplete_prediction_inventory(self):
        self.reject('missing','Incomplete prediction provenance')

    def test_changed_prediction_bytes(self):
        self.reject('changed','Changed prediction cache')


if __name__ == '__main__':
    unittest.main()
