"""Compare independently produced SAM caches without assuming determinism."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np

p = argparse.ArgumentParser()
p.add_argument('first', type=Path)
p.add_argument('second', type=Path)
p.add_argument('output', type=Path)
a = p.parse_args()
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
records = [json.loads((folder / 'evidence.json').read_text()) for folder in (a.first, a.second)]
for key in ('manifest_sha256', 'model', 'source_sha256', 'seed', 'deterministic_algorithms', 'loaded_state_sha256', 'mps_environment', 'torch', 'device', 'runner_sha256'):
    if records[0][key] != records[1][key]:
        raise ValueError('Different run configuration: ' + key)
for record in records:
    inventory=record['output_sha256']
    if not inventory or len(inventory)!=sum(run['frames'] for run in record['runs']):
        raise ValueError('Empty or incomplete prediction inventory')
    for run in record['runs']:
        if sum(name.startswith(run['direction']+'-') for name in inventory)!=run['frames']:
            raise ValueError('Incomplete direction inventory')
if records[0]['output_sha256'].keys() != records[1]['output_sha256'].keys():
    raise ValueError('Different prediction inventory')
frames = []
for name in sorted(records[0]['output_sha256']):
    arrays = []
    for folder, record in zip((a.first, a.second), records):
        path = folder / name
        if sha(path) != record['output_sha256'][name]:
            raise ValueError('Modified prediction: ' + str(path))
        arrays.append(np.load(path, allow_pickle=False))
    x, y = arrays
    if x.shape != y.shape or not np.isfinite(x).all() or not np.isfinite(y).all():
        raise ValueError('Invalid repeated output')
    frames.append(dict(name=name, exact=bool(np.array_equal(x, y)),
                       mae=float(np.abs(x-y).mean()), max_error=float(np.abs(x-y).max()),
                       threshold_disagreement_pixels=int(((x>.5)!=(y>.5)).sum())))
result = dict(first_evidence_sha256=sha(a.first/'evidence.json'),
              second_evidence_sha256=sha(a.second/'evidence.json'),
              configuration_equal=True, bit_exact=all(x['exact'] for x in frames),
              frames=frames, limitation='Two independent processes on one fixed clip/device; not universal determinism certification.')
with a.output.open('x') as f:
    json.dump(result, f, indent=2)
print(json.dumps({k:v for k,v in result.items() if k != 'frames'}, indent=2))
