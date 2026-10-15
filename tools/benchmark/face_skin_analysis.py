"""Research face-skin selection; opaque sRGB, independent IMAGE calls only.

YuNet proposals -> independently confirmed face mesh -> semantic skin intersection.
No identity recognition, tracked state, guessed skin hue or unconfirmed body fallback.
"""
from pathlib import Path
import hashlib
import cv2
import numpy as np
from smart_analysis import SkinSegmenter, validate_rgb, smoothstep, stabilize_mask

FACE_SHA = '64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff'
DETECTOR_SHA = 'ebafce4e3c118d6554634be5c27ab333b4c047a9a8c3faf1d7cf93101c22f0f0'
MAX_FACES = 16


def checked_model(path, expected):
    path = Path(path)
    if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
        raise ValueError('Unrecognized face model bytes')
    return str(path)


def box_iou(a, b):
    low = np.maximum(a[:2], b[:2]); high = np.minimum(a[2:], b[2:])
    overlap = float(np.maximum(high-low, 0).prod())
    union = float((a[2:]-a[:2]).prod()+(b[2:]-b[:2]).prod()-overlap)
    return overlap/union if union > 0 else 0.


def select_boxes(proposals, shape, min_score=.8, minimum_box_size=10.):
    """Stable score-order NMS. Bad model outputs fail, edge faces abstain."""
    h, w = shape
    if not np.isfinite(min_score) or not .6 <= min_score <= 1:
        raise ValueError('Invalid proposal threshold')
    if not np.isfinite(minimum_box_size) or not 1<=minimum_box_size<=10:raise ValueError('Invalid minimum box size')
    valid = []
    for box, score in proposals:
        box = np.asarray(box, dtype=np.float64)
        if box.shape != (4,) or not np.isfinite(box).all() or not np.isfinite(score) or not 0 <= score <= 1:
            raise ValueError('Invalid face proposal')
        size = box[2:]-box[:2]
        if (score < min_score or min(size) < minimum_box_size or box[0] < 0 or box[1] < 0
                or box[2] > w or box[3] > h or not .3 < size[0]/size[1] < 2):
            continue
        valid.append((box, float(score)))
    valid.sort(key=lambda item: (-item[1], *item[0]))
    chosen = []
    for box, score in valid:
        if all(box_iou(box, prev[0]) < .35 for prev in chosen):
            chosen.append((box, score))
            if len(chosen) == MAX_FACES:
                break
    return chosen


def crop_box(box, shape, factor):
    h, w = shape
    center = (box[:2]+box[2:])/2
    side = max(box[2:]-box[:2])*factor
    lo = np.maximum(np.floor(center-side/2).astype(int), 0)
    hi = np.minimum(np.ceil(center+side/2).astype(int), [w, h])
    return int(lo[0]), int(lo[1]), int(hi[0]), int(hi[1])


def face_support(points, shape, oval, exclusions):
    """Face outline with inward feather and eye/lip holes; never a skin proof."""
    points = np.asarray(points, dtype=np.float64)
    h, w = shape
    if points.shape != (478, 2) or not np.isfinite(points).all():
        raise ValueError('Invalid face landmarks')
    outline = points[oval]
    low, high = outline.min(0), outline.max(0)
    size = high-low
    if (min(size) < 8 or low.min() < 0 or high[0] > w-1 or high[1] > h-1
            or not .25 < size[0]/size[1] < 2):
        return np.zeros(shape, np.float32)
    polygon = np.rint(outline).astype(np.int32)
    if abs(cv2.contourArea(polygon)) < .25*float(size.prod()):
        return np.zeros(shape, np.float32)
    raster = np.zeros(shape, np.uint8)
    cv2.fillPoly(raster, [polygon], 1)
    for indices in exclusions:
        # Feature convex hulls deliberately exclude eyelids/lips conservatively.
        pts = points[indices]
        if pts.min() < 0 or pts[:, 0].max() > w-1 or pts[:, 1].max() > h-1:
            return np.zeros(shape, np.float32)
        cv2.fillConvexPoly(raster, cv2.convexHull(np.rint(pts).astype(np.int32)), 0)
    distance = cv2.distanceTransform(raster, cv2.DIST_L2, cv2.DIST_MASK_PRECISE)
    return smoothstep(0, max(2., .04*min(size)), distance).astype(np.float32)


def stabilize_face_mask(images, masks, supports, center, cuts):
    if len(supports) != len(masks):
        raise ValueError('Support window mismatch')
    support = supports[center]
    if (support.shape != images[center].shape[:2] or not np.isfinite(support).all()
            or support.min() < 0 or support.max() > 1):
        raise ValueError('Invalid current face support')
    # Enforce the current mesh/hole boundary after temporal blending. min avoids
    # double attenuation at feathered edges and cannot paint outside support.
    return np.minimum(stabilize_mask(images, masks, center, cuts), support)


class FaceSkinSegmenter:
    def __init__(self, skin_model, face_model, detector_model, *, proposal_threshold=.8, minimum_semantic_mean=0., minimum_box_size=10.):
        if not np.isfinite(proposal_threshold) or not .6 <= proposal_threshold <= .8:
            raise ValueError('Invalid research proposal threshold')
        if not np.isfinite(minimum_box_size) or not 1<=minimum_box_size<=10:raise ValueError('Invalid minimum box size')
        self.minimum_box_size=minimum_box_size
        self.proposal_threshold = proposal_threshold
        if not np.isfinite(minimum_semantic_mean) or not 0 <= minimum_semantic_mean <= 1:
            raise ValueError('Invalid semantic threshold')
        self.minimum_semantic_mean = minimum_semantic_mean
        face_path = checked_model(face_model, FACE_SHA)
        detector_path = checked_model(detector_model, DETECTOR_SHA)
        self.skin = SkinSegmenter(Path(skin_model))
        self.face = None
        try:
            mp = self.skin.mp
            self.face = mp.tasks.vision.FaceLandmarker.create_from_options(
                mp.tasks.vision.FaceLandmarkerOptions(
                    base_options=mp.tasks.BaseOptions(model_asset_path=face_path),
                    running_mode=mp.tasks.vision.RunningMode.IMAGE, num_faces=1,
                    min_face_detection_confidence=.5, min_face_presence_confidence=.5))
            self.detector = cv2.FaceDetectorYN.create(detector_path, '', (320, 320), proposal_threshold, .3, 5000)
            connections = mp.tasks.vision.FaceLandmarksConnections
            self.oval = [edge.start for edge in connections.FACE_LANDMARKS_FACE_OVAL]
            self.exclusions = [sorted({i for edge in getattr(connections, name) for i in (edge.start, edge.end)})
                               for name in ('FACE_LANDMARKS_LEFT_EYE', 'FACE_LANDMARKS_RIGHT_EYE', 'FACE_LANDMARKS_LIPS')]
        except BaseException:
            self.close()
            raise

    def close(self):
        try:
            if self.face is not None:
                self.face.close(); self.face = None
        finally:
            self.skin.close()

    def analyze_with_support(self, rgb, minimum_mesh_height=0.):
        validate_rgb(rgb)
        if not np.isfinite(minimum_mesh_height) or minimum_mesh_height<0:raise ValueError('Invalid mesh height')
        h, w = rgb.shape[:2]
        if min(h, w) < 32 or max(h, w) > 2048 or h*w > 2097152:
            raise ValueError('Research face analysis requires 32..2048 sides and <=2MP')
        bgr = cv2.cvtColor(np.rint(rgb*255).astype(np.uint8), cv2.COLOR_RGB2BGR)
        proposals = []; scale_boxes = {}
        # Two fixed whole-frame scales, no exploratory tile flood. Upscaling is
        # detector preparation, not recovered image detail. Cap runtime input.
        for scale in (min(1., 1536/max(h,w)), min(2., 1536/max(h,w))):
            size = (round(w*scale), round(h*scale))
            scale_boxes.setdefault(size, [])
            sample = cv2.resize(bgr, size, interpolation=cv2.INTER_LINEAR)
            self.detector.setInputSize(size)
            _, detected = self.detector.detect(sample)
            if detected is None:
                continue
            if detected.ndim != 2 or detected.shape[1] != 15 or not np.isfinite(detected).all():
                raise ValueError('Invalid detector result')
            for row in detected:
                x,y,bw,bh = row[:4]
                # Use actual rounded dimensions for coordinate conversion.
                box = np.array([x*w/size[0],y*h/size[1],(x+bw)*w/size[0],(y+bh)*h/size[1]])
                proposals.append((box, float(row[-1])))
                scale_boxes[size].append((box,float(row[-1])))
        faces = select_boxes(proposals, (h,w), getattr(self,'proposal_threshold',.8), getattr(self,'minimum_box_size',10.))
        # Profile agreement requires valid, strong proposals on both distinct
        # scales. Raw out-of-frame/low-score boxes cannot confirm a profile.
        strong_scales = [select_boxes(group,(h,w),.8,getattr(self,'minimum_box_size',10.)) for group in scale_boxes.values()]
        mask = np.zeros((h,w),np.float32); support = mask.copy(); records = []
        for box, score in faces:
            record = dict(box=box.tolist(),detector_score=score,
                          scale_agreement=sum(any(box_iou(box,b)>.5 for b,_ in group) for group in strong_scales))
            if box[3]-box[1]<minimum_mesh_height:
                records.append(dict(**record,status='small_region'));continue
            x0,y0,x1,y1 = crop_box(box,(h,w),2.)
            sample = np.ascontiguousarray(np.rint(rgb[y0:y1,x0:x1]*255).astype(np.uint8))
            result = self.face.detect(self.skin.mp.Image(image_format=self.skin.mp.ImageFormat.SRGB, data=sample))
            if not result.face_landmarks:
                records.append(dict(**record,status='no_mesh')); continue
            mesh = np.asarray([[(p.x*(x1-x0)+x0),(p.y*(y1-y0)+y0)] for p in result.face_landmarks[0]])
            if mesh.shape != (478,2) or not np.isfinite(mesh).all():
                raise ValueError('Invalid face mesh output')
            outline = mesh[self.oval]; meshbox = np.r_[outline.min(0),outline.max(0)]
            if box_iou(box,meshbox) < .35:
                records.append(dict(**record,status='mesh_disagreement')); continue
            region = face_support(mesh,(h,w),self.oval,self.exclusions)
            if not region.any():
                records.append(dict(**record,status='invalid_or_edge_geometry')); continue
            x0,y0,x1,y1 = crop_box(box,(h,w),2.6)
            semantic = self.skin._infer(rgb[y0:y1,x0:x1])
            weight = smoothstep(.45,.85,np.clip(semantic[...,2]+semantic[...,3],0,1))
            local = region[y0:y1,x0:x1]*weight
            semantic_mean = float(local.sum()/max(1,(region>0).sum()))
            if semantic_mean < getattr(self,'minimum_semantic_mean',0.):
                records.append(dict(**record,status='semantic_rejected',semantic_mean_inside=semantic_mean)); continue
            mask[y0:y1,x0:x1] = np.maximum(mask[y0:y1,x0:x1],local)
            support = np.maximum(support,region)
            records.append(dict(**record,status='confirmed',semantic_mean_inside=semantic_mean))
        detail = dict(faces=records,proposals=len(proposals),face_count=sum(r['status']=='confirmed' for r in records),
                      area=float(mask.mean()),scope='face skin only; body/hands excluded')
        return mask,support,detail

    def analyze(self,rgb):
        mask,_,detail = self.analyze_with_support(rgb)
        return mask,detail
