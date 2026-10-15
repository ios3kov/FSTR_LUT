"""Experimental visible-face gate. Local pinned FaceOcc; never a mask source.

Input: opaque sRGB float RGB [0,1]. Current-frame boxes in pixel coordinates.
No inference in AE, identity association, or temporal state. Research only.
"""
from pathlib import Path
import hashlib
import cv2
import numpy as np
from smart_analysis import validate_rgb

MODEL_SHA = '5d6d27a9cb221692425840a80eb4efc07e6245670b233e2eb618e83fd9b7caf6'


def face_crop(box, shape, factor=1.5):
    """Square face context with explicit inverse map, including edge padding."""
    b=np.asarray(box,dtype=np.float64)
    if b.shape!=(4,) or not np.isfinite(b).all():raise ValueError('Invalid face box')
    h,w=shape;size=b[2:]-b[:2]
    if min(size)<8 or b.min()<0 or b[2]>w or b[3]>h:raise ValueError('Out-of-frame face box')
    if factor not in (1.5,2.):raise ValueError('Unsupported research crop factor')
    center=(b[:2]+b[2:])/2;side=float(max(size)*factor)
    # Map source pixel coordinates to256x256; inverse uses exactly this matrix.
    return np.array([[256/side,0,128-center[0]*256/side],[0,256/side,128-center[1]*256/side]],np.float32)


def limit_mask(mask,visibility):
    if mask.ndim!=2 or mask.shape!=visibility.shape:raise ValueError('Gate shape mismatch')
    for x in (mask,visibility):
        if not np.isfinite(x).all() or x.min()<0 or x.max()>1:raise ValueError('Invalid gate values')
    # Canonical upstream threshold. No confidence calibration or soft threshold
    # tuning claimed. Temporal smoothing must never reintroduce rejected pixels.
    return np.where(visibility>=.5,mask,0).astype(np.float32)


class VisibleFaceSegmenter:
    def __init__(self,path):
        path=Path(path)
        with path.open('rb') as f:digest=hashlib.file_digest(f,'sha256').hexdigest()
        if digest!=MODEL_SHA:raise ValueError('Unrecognized visible-face weights')
        import torch
        import segmentation_models_pytorch as smp
        from safetensors.torch import load_file
        self.torch=torch
        self.model=smp.Unet(encoder_name='resnet18',encoder_weights=None,in_channels=3,classes=1)
        self.model.load_state_dict(load_file(str(path)),strict=True);self.model.eval()

    def analyze(self,rgb,boxes,factor=1.5):
        validate_rgb(rgb)
        if max(rgb.shape[:2])>2048 or len(boxes)>16:raise ValueError('Research input limit')
        if factor not in (1.5,2.):raise ValueError('Unsupported research crop factor')
        h,w=rgb.shape[:2];result=np.zeros((h,w),np.float32)
        for box in boxes:
            transform=face_crop(box,(h,w),factor)
            # Fixed roll hypotheses reduce pose sensitivity. Their maximum is
            # an uncalibrated visibility score; it can also admit false positives.
            for angle in (-30,0,30):
                rotated=(cv2.getRotationMatrix2D((128,128),angle,1)@np.vstack((transform,[0,0,1]))).astype(np.float32)
                sample=cv2.warpAffine(rgb.astype(np.float32,copy=False),rotated,(256,256),flags=cv2.INTER_LINEAR,borderMode=cv2.BORDER_REFLECT_101)
                x=(sample-np.array([.485,.456,.406],np.float32))/np.array([.229,.224,.225],np.float32)
                tensor=self.torch.from_numpy(np.ascontiguousarray(x.transpose(2,0,1)[None]))
                with self.torch.inference_mode():prob=self.model(tensor).sigmoid().numpy()[0,0]
                if prob.shape!=(256,256) or not np.isfinite(prob).all():raise ValueError('Invalid visible-face output')
                restored=cv2.warpAffine(prob,rotated,(w,h),flags=cv2.INTER_LINEAR|cv2.WARP_INVERSE_MAP,borderMode=cv2.BORDER_CONSTANT)
                result=np.maximum(result,restored)
        return result
