"""Explicit bounded anchor interpolation of added color, never source RGB."""
import numpy as np


def interpolate_correction(current, left, right, fraction, support):
    current, left, right = [np.asarray(v, dtype=np.float64) for v in (current, left, right)]
    support = np.asarray(support, dtype=np.float64)
    if current.shape != left.shape or current.shape != right.shape or current.shape[-1] != 3 or support.shape != current.shape[:-1]:
        raise ValueError('Correction/support shape')
    if not all(np.isfinite(v).all() for v in (current, left, right, support)) or not np.isfinite(fraction) or not 0 <= fraction <= 1 or np.any((support < 0) | (support > 1)):
        raise ValueError('Finite bounded anchor values required')
    target = (1-fraction)*left + fraction*right
    return ((1-support[..., None])*current + support[..., None]*target).astype(np.float32)


def anchor_pair(index, anchors, present, cut_before=(), max_span=4):
    """No extrapolation, absent identity, cuts or long/unobserved gaps."""
    if max_span < 2 or index not in present or index in anchors:
        return None
    left = max((v for v in anchors if v < index), default=None)
    right = min((v for v in anchors if v > index), default=None)
    if left is None or right is None or right-left > max_span or any(v in cut_before for v in range(left+1, right+1)) or any(v not in present for v in range(left, right+1)):
        return None
    return left, right, (index-left)/(right-left)


def stable_strengths(values, confidence, cut_before=(), max_span=4):
    """Per-person scalar schedule; no RGB or spatial masks in temporal averaging.

    All samples must belong to the same supplied identity. Missing samples and
    cuts split runs. Anchors every max_span frames use local median strength.
    Low-confidence anchors leave the original strength unchanged.
    """
    if max_span < 2 or set(values) != set(confidence):
        raise ValueError('Strength schedule contract')
    if any(not np.isfinite(v) or not 0 <= v <= 1 for v in (*values.values(), *confidence.values())):
        raise ValueError('Strength/confidence requires [0,1]')
    result=dict(values); used={}; runs=[]
    for i in sorted(values):
        if not runs or i != runs[-1][-1]+1 or i in cut_before:runs.append([])
        runs[-1].append(i)
    for run in runs:
        candidates=run[::max_span]
        if run[-1] not in candidates:candidates.append(run[-1])
        anchors={}
        for i in candidates:
            neighbors=[j for j in (i-1,i,i+1) if j in run and confidence[j]>=.8]
            if confidence[i]>=.8 and neighbors:anchors[i]=float(np.median([values[j] for j in neighbors]))
        for i in run:
            if i in anchors:result[i]=anchors[i];used[i]=(i,i,0.)
            else:
                pair=anchor_pair(i,anchors,set(run),cut_before,max_span)
                if pair:
                    l,r,f=pair;result[i]=(1-f)*anchors[l]+f*anchors[r];used[i]=pair
    return result,used
