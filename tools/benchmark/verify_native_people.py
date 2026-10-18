"""Native YuNet, mesh and person masks against independent official runtimes."""
import argparse,json,subprocess,hashlib
from pathlib import Path
import cv2,numpy as np,mediapipe as mp
p=argparse.ArgumentParser()
for k in ['probe','runtime','skin','face','pose','detector','parts','output']:p.add_argument('--'+k,type=Path,required=True)
a=p.parse_args();a.output.mkdir(exist_ok=False);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();records=[]
face=mp.tasks.vision.FaceLandmarker.create_from_options(mp.tasks.vision.FaceLandmarkerOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(a.face.resolve())),num_faces=1,running_mode=mp.tasks.vision.RunningMode.IMAGE))
pose=mp.tasks.vision.PoseLandmarker.create_from_options(mp.tasks.vision.PoseLandmarkerOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(a.pose.resolve())),num_poses=4,output_segmentation_masks=True,running_mode=mp.tasks.vision.RunningMode.IMAGE))
try:
 for i,case in enumerate(json.loads((a.parts/'verification.json').read_text())['cases']):
  h,w,_=case['shape'];raw=a.parts/f'{i}.rgba8';rgb=np.fromfile(raw,'uint8').reshape(h,w,4)[...,:3].copy();out=a.output/str(i)
  run=subprocess.run([str(a.probe.resolve()),str(a.runtime.resolve()),sha(a.runtime),str(a.skin.resolve()),str(a.face.resolve()),str(a.pose.resolve()),str(a.detector.resolve()),str(w),str(h),str(raw.resolve()),str(out.resolve())],capture_output=True,text=True);(a.output/f'{i}.stderr').write_text(run.stderr)
  if run.returncode:raise RuntimeError(run.stderr)
  got=json.loads((out/'result.json').read_text());meshes=[]
  for f in got['faces']:
   x0,y0,x1,y1=f['crop'];result=face.detect(mp.Image(image_format=mp.ImageFormat.SRGB,data=rgb[y0:y1,x0:x1].copy()));expected=np.array([[[l.x,l.y,l.z] for l in row] for row in result.face_landmarks],np.float32)
   actual=np.array(f['mesh'],np.float32)
   if len(expected)!=bool(len(actual)):raise ValueError('Mesh presence mismatch')
   err=float(np.abs(expected[0]-actual).max()) if len(actual) else 0;meshes.append(err)
   if err>1e-6:raise ValueError('Mesh differs')
  result=pose.detect(mp.Image(image_format=mp.ImageFormat.SRGB,data=rgb));masks=result.segmentation_masks or []
  if got['poses']!=len(masks):raise ValueError('Pose count differs')
  errors=[float(np.abs(np.fromfile(out/f'pose-{j}.f32',dtype='<f4').reshape(h,w)-m.numpy_view().squeeze()).max()) for j,m in enumerate(masks)]
  if max(errors,default=0)>1e-6:raise ValueError('Pose differs')
  # Verify detector counts/boxes against existing OpenCV YuNet at two scales.
  detector=cv2.FaceDetectorYN.create(str(a.detector/'yunet.onnx'),'',(w,h),.8,.3,5000);proposals=[]
  for scale in [1,2]:
   size=(round(w*scale),round(h*scale));detector.setInputSize(size);_,rows=detector.detect(cv2.resize(rgb[...,::-1],size))
   if rows is not None:
    for row in rows:
     x,y,bw,bh=row[:4];box=np.array([x,y,x+bw,y+bh])/scale
     if min(box)<0 or box[2]>w or box[3]>h or min(bw,bh)/scale<1 or not .3<bw/bh<2:continue
     proposals.append((box,float(row[-1])))
  chosen=[]
  def iou(a,b):
   lo=np.maximum(a[:2],b[:2]);hi=np.minimum(a[2:],b[2:]);inter=np.maximum(hi-lo,0).prod();return inter/max(1e-12,np.prod(a[2:]-a[:2])+np.prod(b[2:]-b[:2])-inter)
  for box,score in sorted(proposals,key=lambda x:(-x[1],*x[0])):
   if all(iou(box,b)<.35 for b,_ in chosen):chosen.append((box,score))
  chosen.sort(key=lambda x:tuple(x[0]));diff=max((float(np.abs(b-f['box']).max()) for (b,_),f in zip(chosen,got['faces'])),default=0)
  if len(chosen)!=len(got['faces']) or diff>1:raise ValueError(f'Detector mismatch: {len(chosen)} vs {len(got["faces"])}; delta {diff}')
  row={'source':case['source'],'faces':len(chosen),'detector_max_box_px':diff,'mesh_max_abs':max(meshes,default=0),'poses':len(masks),'pose_max_abs':max(errors,default=0),'detector_tolerance_pixels':1,'model_tolerance':1e-6};records.append(row);print(row,flush=True)
finally:face.close();pose.close()
(a.output/'verification.json').write_text(json.dumps({'cases':records,'probe_sha256':sha(a.probe),'runtime_sha256':sha(a.runtime),'face_sha256':sha(a.face),'pose_sha256':sha(a.pose)},indent=2))
