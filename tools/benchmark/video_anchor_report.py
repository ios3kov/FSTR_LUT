"""Report common-region diagnostics; these are not perceptual acceptance."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np

p = argparse.ArgumentParser()
p.add_argument('trial', type=Path)
p.add_argument('--comparison', default='comparison-reviewed')
p.add_argument('--sam2', default='sam2-final')
a = p.parse_args()
root = a.trial / a.comparison
evidence = json.loads((root / 'evidence.json').read_text())
manifest = json.loads((a.trial / 'manifest.json').read_text())
rows = []
for case, source in zip(evidence['cases'], manifest['cases']):
    if not case['supported']:
        continue
    index = source['index']
    forward = np.load(a.trial / a.sam2 / f'forward-{index:03}.npy', allow_pickle=False)
    backward = np.load(a.trial / a.sam2 / f'backward-{index:03}.npy', allow_pickle=False)
    roi = np.minimum(forward, backward) > .8
    row = dict(frame=case['frame'], common_roi_pixels=int(roi.sum()))
    for mode in ('baseline', 'flow', 'sam2', 'hybrid'):
        field = np.load(root / f'{source["name"]}-{mode}-mask.npy', allow_pickle=False)
        row[mode] = float(field[..., 0][roi].mean()) if roi.any() else None
    contribution = np.load(root / f'{source["name"]}-flow-contribution.npy', allow_pickle=False)
    row['flow_contribution_pixels_over_001'] = int((contribution > .01).sum())
    rows.append(row)
result = dict(
    description='Same SAM consensus region across methods. Model-selected diagnostic, not ground truth or a flicker score. Thresholds tuned on this clip.',
    comparison_evidence_sha256=hashlib.sha256((root / 'evidence.json').read_bytes()).hexdigest(),
    frames=rows,
)
(root / 'metrics.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result, indent=2))
