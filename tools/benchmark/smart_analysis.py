"""Host-independent research analysis. Explicit SDR, no hidden video history.

All images are opaque sRGB float RGB [0,1]. Temporal API takes the complete
bounded input window and cut barriers. Model runtime is only loaded on request.
"""
from __future__ import annotations
import hashlib
import zipfile
from pathlib import Path
import cv2
import numpy as np

MODEL_SHA = 'c6748b1253a99067ef71f7e26ca71096cd449baefa8f101900ea23016507e0e0'
LABELS = ['background', 'hair', 'body-skin', 'face-skin', 'clothes', 'others']
LUMA = np.array([.2126, .7152, .0722])


def validate_rgb(rgb):
    if (rgb.ndim != 3 or rgb.shape[2] != 3 or min(rgb.shape[:2]) < 1
            or not np.isfinite(rgb).all() or rgb.min() < 0 or rgb.max() > 1):
        raise ValueError('Analysis requires finite opaque SDR RGB [0,1]')


def decode_srgb(rgb):
    return np.where(rgb <= .04045, rgb / 12.92, ((rgb + .055) / 1.055) ** 2.4)


def encode_srgb(rgb):
    # Display-copy conversion only. Core output remains unclipped.
    x = np.maximum(rgb, 0)
    return np.where(x <= .0031308, 12.92*x, 1.055*x**(1/2.4)-.055)


def smoothstep(lo, hi, x):
    t = np.clip((x-lo)/(hi-lo), 0, 1)
    return t*t*(3-2*t)


class SkinSegmenter:
    def __init__(self, model: Path):
        if hashlib.sha256(model.read_bytes()).hexdigest() != MODEL_SHA:
            raise ValueError('Unrecognized model bytes')
        with zipfile.ZipFile(model) as z:
            if z.read('labels.txt').decode().splitlines() != LABELS:
                raise ValueError('Unexpected model labels')
        import mediapipe as mp
        self.mp = mp
        self.runtime = mp.tasks.vision.ImageSegmenter.create_from_options(
            mp.tasks.vision.ImageSegmenterOptions(
                base_options=mp.tasks.BaseOptions(model_asset_path=str(model)),
                running_mode=mp.tasks.vision.RunningMode.IMAGE,
                output_confidence_masks=True))

    def close(self):
        self.runtime.close()

    def _infer(self, image):
        h, w = image.shape[:2]
        scale = min(1., 384/max(w, h))
        sample = cv2.resize(image, (max(1, round(w*scale)), max(1, round(h*scale))), interpolation=cv2.INTER_AREA)
        result = self.runtime.segment(self.mp.Image(
            image_format=self.mp.ImageFormat.SRGB,
            data=np.ascontiguousarray(np.rint(sample*255).astype(np.uint8))))
        if len(result.confidence_masks) != 6:
            raise ValueError('Unexpected confidence channel count')
        maps = np.stack([m.numpy_view().squeeze().copy() for m in result.confidence_masks], -1)
        if not np.isfinite(maps).all() or maps.min() < 0 or maps.max() > 1:
            raise ValueError('Invalid model output')
        if maps.shape[:2] != sample.shape[:2]:
            raise ValueError('Unexpected model mask dimensions')
        return cv2.resize(maps, (w, h), interpolation=cv2.INTER_LINEAR)

    def analyze(self, rgb):
        validate_rgb(rgb)
        h, w = rgb.shape[:2]
        maps = self._infer(rgb)
        coarse = np.clip(maps[..., 2]+maps[..., 3], 0, 1)
        refined = coarse.copy()
        crops = []
        # Fixed overlapping semantic views also cover people missed completely
        # by the full-frame model. Their geometry does not jump with detection.
        # This is mask analysis, not tiled image restoration; quality is measured.
        cw, ch = max(1, round(w*2/3)), max(1, round(h*2/3))
        accumulator = np.zeros((h,w),np.float64)
        support = np.zeros((h,w),np.float64)
        for y0 in sorted(set([0,h-ch])):
            for x0 in sorted(set([0,w-cw])):
                x1,y1=x0+cw,y0+ch
                local=self._infer(rgb[y0:y1,x0:x1])
                skin=np.clip(local[...,2]+local[...,3],0,1)
                ry,rx=np.mgrid[:ch,:cw]
                # Feather interior tile borders; real image borders stay valid.
                edge=np.ones((ch,cw),np.float64)
                for distance,active in [(rx,x0>0),(cw-1-rx,x1<w),
                                        (ry,y0>0),(ch-1-ry,y1<h)]:
                    if active:edge*=smoothstep(0,12,distance)
                accumulator[y0:y1,x0:x1]+=skin*edge
                support[y0:y1,x0:x1]+=edge
                crops.append([x0,y0,x1,y1])
        tiled=np.divide(accumulator,support,out=np.zeros_like(accumulator),where=support>0)
        refined=np.maximum(refined,tiled).astype(np.float32)
        weight = smoothstep(.45, .85, refined).astype(np.float32)
        return weight, dict(crops=crops, coarse_peak=float(coarse.max()),
                           refined_peak=float(refined.max()), area=float(weight.mean()))


def estimate_balance(rgb):
    """Conservative aesthetic heuristic; uncertainty returns exact identity."""
    validate_rgb(rgb)
    small = cv2.resize(rgb, (128, 128), interpolation=cv2.INTER_AREA)
    linear = decode_srgb(small)
    y = linear @ LUMA
    q05, q50, q90, q98 = np.quantile(y, [.05, .5, .9, .98])
    balance = np.array([0., 1., 1., 1.], dtype=np.float64)
    reason = 'identity'
    # Abstain when even the upper quantile is dark or the frame is uniform.
    # A dark shot with bright objects may still receive a modest lift; this
    # heuristic cannot establish the intended lighting of a night scene.
    if q90 >= .18 and q98 < .8 and .015 < q50 < .08 and q90-q05 > .1:
        balance[0] = min(.3, .3*(.08-q50)/.065)
        reason = 'bounded-shadow-exposure'
    hi, lo = linear.max(-1), linear.min(-1)
    chroma = (hi-lo)/np.maximum(hi, 1e-6)
    neutral = (chroma < .12) & (y > .06) & (y < .7) & (hi < .9)
    support = float(neutral.mean())
    cells = sum(bool(neutral[yy:yy+32, xx:xx+32].mean() > .03)
                for yy in range(0, 128, 32) for xx in range(0, 128, 32))
    wb_used = False
    if support >= .04 and cells >= 6 and q90-q05 > .06 and q90 > .18:
        values = np.log(np.maximum(linear[neutral], 1e-6))
        centered = values-values.mean(-1, keepdims=True)
        ratios = np.median(centered, axis=0)
        disagreement = np.max(np.median(np.abs(centered-ratios), axis=0))
        cast = np.max(np.abs(ratios))
        if disagreement < .025 and .015 < cast < .075:
            gains = np.exp(-.4*ratios)
            gains /= gains @ LUMA
            if gains.min() >= .95 and gains.max() <= 1.05:
                balance[1:] = gains
                wb_used = True
    return balance, dict(reason=reason, neutral_support=support, cells=cells,
                         wb_applied=wb_used, q05=float(q05), q50=float(q50),
                         q90=float(q90), q98=float(q98))


def cut_between(a, b):
    # Intentionally conservative. Flash can be classified as a cut; no crossfade.
    a = cv2.resize(a, (32, 32), interpolation=cv2.INTER_AREA)
    b = cv2.resize(b, (32, 32), interpolation=cv2.INTER_AREA)
    return bool(np.abs(a-b).mean() > .18)


def connected_indices(center, size, cuts, radius=2):
    if not 0 <= center < size or len(cuts) != size-1 or radius != 2:
        raise ValueError('Explicit radius2 window/cut boundaries required')
    return [j for j in range(max(0, center-radius), min(size, center+radius+1))
            if not any(cuts[min(center, j):max(center, j)])]


def stabilize_balance(balances, center, cuts):
    indices = connected_indices(center, len(balances), cuts)
    b = np.asarray([balances[j] for j in indices], dtype=np.float64)
    if (b.shape[1:] != (4,) or not np.isfinite(b).all() or np.any(np.abs(b[:, 0]) > .5)
            or np.any(b[:, 1:] < .9) or np.any(b[:, 1:] > 1.1)):
        raise ValueError('Invalid balance window')
    weights = np.array([1. if j == center else .15 for j in indices])
    weights /= weights.sum()
    result = np.r_[weights @ b[:, 0], np.exp(weights @ np.log(b[:, 1:]))]
    # An identity/uncertain center never borrows a correction from neighbors.
    # Zero estimates still participate in smoothing a supported center.
    if balances[center][0] == 0:
        result[0] = 0
    if np.array_equal(balances[center][1:], [1,1,1]):
        result[1:] = 1
    return result


def stabilize_mask(images, masks, center, cuts):
    """Flow-warp explicit neighbors; current has >=62.5% of total weight."""
    indices = connected_indices(center, len(images), cuts)
    if len(masks) != len(images):
        raise ValueError('Mask window dimensions mismatch')
    current = masks[center]
    for j in indices:
        validate_rgb(images[j])
        m = masks[j]
        if (m.shape != images[center].shape[:2] or images[j].shape != images[center].shape
                or not np.isfinite(m).all() or m.min() < 0 or m.max() > 1):
            raise ValueError('Invalid bounded mask/image window')
    h, w = current.shape
    acc, denom = current.astype(np.float64).copy(), np.ones((h, w), np.float64)
    gray = np.rint(cv2.cvtColor(images[center], cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
    yy, xx = np.mgrid[:h, :w].astype(np.float32)
    for j in indices:
        if j == center:
            continue
        other = np.rint(cv2.cvtColor(images[j], cv2.COLOR_RGB2GRAY)*255).astype(np.uint8)
        forward = cv2.calcOpticalFlowFarneback(gray, other, None, .5, 3, 15, 3, 5, 1.2, 0)
        backward = cv2.calcOpticalFlowFarneback(other, gray, None, .5, 3, 15, 3, 5, 1.2, 0)
        mx, my = xx+forward[..., 0], yy+forward[..., 1]
        remap = lambda a: cv2.remap(a, mx, my, cv2.INTER_LINEAR, borderMode=cv2.BORDER_CONSTANT)
        residual = np.linalg.norm(forward+remap(backward), axis=-1)
        photo = np.abs(images[center]-remap(images[j])).mean(-1)
        valid = ((mx >= 0) & (mx <= w-1) & (my >= 0) & (my <= h-1)
                 & (residual < 1) & (photo < .08))
        weight = .15*valid*(1-smoothstep(.25, 1., residual))
        # Prevent temporal protection from being painted onto newly revealed skin-free areas.
        warped = remap(masks[j])
        valid_support = (current > .02) | (warped <= current)
        weight *= valid_support
        acc += weight*warped
        denom += weight
    return (acc/denom).astype(np.float32)
