"""Independent current-frame people/face evidence. Local research, not AE."""
import argparse,hashlib,json,time
from pathlib import Path
import numpy as np
from PIL import Image
from face_skin_analysis import FaceSkinSegmenter,FACE_SHA,DETECTOR_SHA
from person_head_analysis import POSE_SHA
from smart_analysis import MODEL_SHA,LABELS
from scene_people import face_field,person_field

p=argparse.ArgumentParser()
for key in ('cases','parts','skin-model','face-model','detector-model','pose-model','output'):p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();a.output.mkdir(exist_ok=False)
if hashlib.sha256(a.pose_model.read_bytes()).hexdigest()!=POSE_SHA:raise ValueError('Unknown pose model')
engine=FaceSkinSegmenter(a.skin_model,a.face_model,a.detector_model,minimum_box_size=1.);pose=None;records=[]
try:
 mp=engine.skin.mp
 pose=mp.tasks.vision.PoseLandmarker.create_from_options(mp.tasks.vision.PoseLandmarkerOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(a.pose_model)),running_mode=mp.tasks.vision.RunningMode.IMAGE,num_poses=4,output_segmentation_masks=True,min_pose_detection_confidence=.5,min_pose_presence_confidence=.5))
 for case in json.loads(a.cases.read_text())['cases']:
  name=case['name'];path=Path(case['path'])
  if Path(name).name!=name or hashlib.sha256(path.read_bytes()).hexdigest()!=case['sha256']:raise ValueError('Invalid case/source')
  im=Image.open(path)
  if im.mode not in ('RGB','L'):raise ValueError('Opaque input required')
  rgb=np.array(im.convert('RGB'),dtype=np.float32)/255
  scale=float(case['source_height'])/rgb.shape[0]
  pp=a.parts/(name+'.npy');binding=json.loads((a.parts/(name+'.json')).read_text())
  if binding['source_sha256']!=case['sha256'] or binding['parts_sha256']!=hashlib.sha256(pp.read_bytes()).hexdigest() or binding['model_sha256']!=MODEL_SHA or binding['labels']!=LABELS:raise ValueError('Parts provenance mismatch')
  start=time.perf_counter();mask,support,detail=engine.analyze_with_support(rgb,minimum_mesh_height=64/scale)
  face,face_detail=face_field(rgb,mask,detail['faces'],scale)
  result=pose.detect(mp.Image(image_format=mp.ImageFormat.SRGB,data=np.ascontiguousarray(np.rint(rgb*255).astype(np.uint8))))
  masks=[m.numpy_view().copy().squeeze() for m in (result.segmentation_masks or [])]
  people,person_detail=person_field(rgb,np.load(pp,allow_pickle=False),masks,scale)
  out=a.output/(name+'.npz');np.savez_compressed(out,face=face,people=people,raw_face_mask=mask,raw_face_support=support,raw_pose_masks=np.stack(masks) if masks else np.empty((0,*rgb.shape[:2]),np.float32))
  record=dict(name=name,source_sha256=case['sha256'],source_height=case['source_height'],parts=binding,evidence_sha256=hashlib.sha256(out.read_bytes()).hexdigest(),models=dict(face=FACE_SHA,detector=DETECTOR_SHA,pose=POSE_SHA,skin=MODEL_SHA),seconds=time.perf_counter()-start,faces=face_detail,people=person_detail,raw_face_details=detail,pose_capacity=4)
  (a.output/(name+'.json')).write_text(json.dumps(record,indent=2));records.append(record);print(name,len(face_detail),len(masks),flush=True)
finally:
 if pose is not None:pose.close()
 engine.close()
(a.output/'evidence.json').write_text(json.dumps(records,indent=2))
