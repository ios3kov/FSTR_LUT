"""Optional person-part evidence in the existing isolated MediaPipe runtime."""
import argparse,json,hashlib
from pathlib import Path
import numpy as np
from PIL import Image
from smart_analysis import SkinSegmenter,MODEL_SHA,LABELS
p=argparse.ArgumentParser();p.add_argument('images',type=Path);p.add_argument('model',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(exist_ok=True)
s=SkinSegmenter(a.model)
try:
 for path in sorted(a.images.glob('*.png')):
  im=Image.open(path)
  if im.mode not in ('RGB','L'):raise ValueError('Opaque RGB/L input required')
  rgb=np.array(im.convert('RGB'),dtype=np.float32)/255
  out=a.output/(path.stem+'.npy');np.save(out,s._infer(rgb))
  (a.output/(path.stem+'.json')).write_text(json.dumps(dict(source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),parts_sha256=hashlib.sha256(out.read_bytes()).hexdigest(),model_sha256=MODEL_SHA,labels=LABELS)))
  print(path.name,flush=True)
finally:s.close()
