"""Research-only independent OpenColorIO oracle; never imported by the effect."""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

import numpy as np


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('renderer', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--reference-modules', type=Path, required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(args.reference_modules.resolve()))
    import PyOpenColorIO as ocio
    if ocio.__version__ != '2.4.2':
        raise ValueError('Pinned OpenColorIO2.4.2 required')
    args.output.mkdir(exist_ok=False)
    renderer = args.renderer.resolve()

    def xyz(w):
        x, y = w
        return np.array([x / y, 1., (1 - x - y) / y])

    d60, d65 = xyz((.32168, .33767)), xyz((.3127, .329))
    primaries = np.array([xyz(p) for p in [( .64, .33), (.3, .6), (.15, .06)]]).T
    xyz709 = primaries @ np.diag(np.linalg.solve(primaries, d65))
    def adapt(cones):
        return np.linalg.inv(cones) @ np.diag((cones @ d65) / (cones @ d60)) @ cones
    bradford = adapt(np.array([[.8951, .2664, -.1614], [-.7502, 1.7135, .0367], [.0389, -.0685, 1.0296]]))
    cat02 = adapt(np.array([[.7328, .4296, -.1624], [-.7036, 1.6975, .0061], [.003, .0136, .9834]]))
    styles = {
        'arri-logc3-ei800': 'ARRI_ALEXA-LOGC-EI800-AWG_to_ACES2065-1',
        'arri-logc4': 'ARRI_LOGC4_to_ACES2065-1',
        'panasonic-vlog': 'PANASONIC_VLOG-VGAMUT_to_ACES2065-1',
        'acescct': 'ACEScct_to_ACES2065-1',
        'acescg': 'ACEScg_to_ACES2065-1',
    }
    rng = np.random.default_rng(4129)
    encoded = np.vstack([np.repeat(np.array([-.05, 0., .0729055341958355, .092809, 95/1023, .125, .149658, .155251141552511, .181, .391, .423, 1.])[:, None], 3, axis=1), [[1, 0, 0], [0, 1, 0], [0, 0, 1]], rng.uniform(-.05, .95, (128, 3))])
    records = []
    for key, style in styles.items():
        rgb = encoded.copy()
        if key == 'acescg':
            rgb = np.vstack([rgb, [[-.2, 2, 4], [16, 1, 0], [1, 1, 1]]])
        rgba = np.column_stack([rgb, np.ones(len(rgb))]).astype(np.float32)
        source = args.output / (key + '-input.rgba32f')
        target = args.output / (key + '-output.rgba32f')
        rgba.tofile(source)
        group = ocio.GroupTransform()
        group.appendTransform(ocio.BuiltinTransform(style=style))
        group.appendTransform(ocio.BuiltinTransform(style='UTILITY - ACES-AP0_to_CIE-XYZ-D65_BFD'))
        # ARRI IDTs use CAT02. Undo reference Bradford and CAT02 in the
        # appropriate order to compare direct D65 source -> D65 working RGB.
        matrix = np.linalg.inv(xyz709)
        if key.startswith('arri-'):
            matrix = matrix @ cat02 @ np.linalg.inv(bradford)
        matrix4 = np.eye(4); matrix4[:3, :3] = matrix
        group.appendTransform(ocio.MatrixTransform(matrix=matrix4.ravel().tolist()))
        cpu = ocio.Config.CreateRaw().getProcessor(group).getOptimizedCPUProcessor(ocio.OPTIMIZATION_NONE)
        expected = np.array([cpu.applyRGB(row[:3].tolist()) for row in rgba])
        subprocess.run([str(renderer), str(len(rgba)), '1', str(source), str(target), 'linear', key, '0', '1', '1', '0', 'none', '0', '1', 'straight'], check=True, capture_output=True)
        actual = np.fromfile(target, np.float32).reshape(-1, 4)
        atol, rtol = (3e-5, 1e-4) if key == 'arri-logc3-ei800' else (2e-5, 2e-5)
        delta = np.abs(actual[:, :3] - expected)
        passed = bool(np.all(delta <= atol + rtol * np.max(np.abs(expected), axis=1, keepdims=True)) and np.all(actual[:, 3] == 1))
        expected.astype(np.float32).tofile(args.output / (key + '-oracle.rgb32f'))
        records.append(dict(profile=key, passed=passed, samples=len(rgba), maximum_absolute_error=float(delta.max()), atol=atol, rtol=rtol, scale='per-pixel RGB infinity norm (matrix cancellation)', style=style))
    receipt = dict(result='PASS' if all(r['passed'] for r in records) else 'FAIL', reference='OpenColorIO2.4.2 CPU, no optimization', renderer_sha256=hashlib.sha256(renderer.read_bytes()).hexdigest(), producer_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), profiles=records, limits='Numeric source conversion only; not an ACES output transform, camera-footage or host/visual approval')
    (args.output / 'verification.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))
    return 0 if receipt['result'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
