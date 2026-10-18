"""Independent native C-ABI -> official Python wrapper comparison (tooling only)."""
import argparse,hashlib,json,subprocess,time
from pathlib import Path
import numpy as np
from PIL import Image
import mediapipe as mp
p=argparse.ArgumentParser()
for k in ['probe','runtime','model','output']:p.add_argument('--'+k,type=Path,required=True)
p.add_argument('images',type=Path,nargs='+');a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
options=mp.tasks.vision.ImageSegmenterOptions(base_options=mp.tasks.BaseOptions(model_asset_path=str(a.model.resolve())),running_mode=mp.tasks.vision.RunningMode.IMAGE,output_confidence_masks=True)
runtime=mp.tasks.vision.ImageSegmenter.create_from_options(options);rows=[]
try:
 for i,path in enumerate(a.images):
  im=Image.open(path).convert('RGB');im.thumbnail((384,384),Image.Resampling.BOX);rgb=np.array(im);h,w=rgb.shape[:2];rgba=np.concatenate([rgb,np.full((h,w,1),255,np.uint8)],-1)
  raw=a.output/f'{i}.rgba8';out=a.output/f'{i}.parts';rgba.tofile(raw);start=time.perf_counter()
  run=subprocess.run([str(a.probe.resolve()),str(a.runtime.resolve()),sha(a.runtime),str(a.model.resolve()),str(w),str(h),str(raw.resolve()),str(out.resolve())],capture_output=True,text=True)
  (a.output/f'{i}.stderr').write_text(run.stderr)
  if run.returncode:raise RuntimeError(run.stderr)
  native_seconds=time.perf_counter()-start
  result=runtime.segment(mp.Image(image_format=mp.ImageFormat.SRGB,data=rgb));expected=np.stack([m.numpy_view().squeeze().copy() for m in result.confidence_masks],-1);expected.astype('<f4').tofile(a.output/f'{i}.reference.parts');actual=np.fromfile(out,dtype='<f4').reshape(h,w,6)
  err=float(np.abs(actual-expected).max());row={'source':str(path.resolve()),'source_sha256':sha(path),'shape':[h,w,6],'max_abs':err,'tolerance':1e-6,'native_cold_seconds':native_seconds};rows.append(row);print(row,flush=True)
  if err>1e-6:raise ValueError('Native model mismatch')
finally:runtime.close()
(a.output/'verification.json').write_text(json.dumps({'runtime':str(a.runtime.resolve()),'runtime_sha256':sha(a.runtime),'model_sha256':sha(a.model),'probe_sha256':sha(a.probe),'cases':rows},indent=2))
