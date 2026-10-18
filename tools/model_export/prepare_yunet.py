"""Declare YuNet's existing convolutional spatial dimensions as dynamic."""
import argparse,hashlib,json
from pathlib import Path
import onnx
p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args();a.output.mkdir(exist_ok=False)
sha=hashlib.sha256(a.source.read_bytes()).hexdigest()
if sha!='ebafce4e3c118d6554634be5c27ab333b4c047a9a8c3faf1d7cf93101c22f0f0':raise ValueError('Unknown YuNet')
m=onnx.load(a.source)
for i,name in [(2,'height'),(3,'width')]:d=m.graph.input[0].type.tensor_type.shape.dim[i];d.ClearField('dim_value');d.dim_param=name
for o in m.graph.output:d=o.type.tensor_type.shape.dim[1];d.ClearField('dim_value');d.dim_param=o.name+'_anchors'
onnx.checker.check_model(m);onnx.save(m,a.output/'yunet.onnx')
(a.output/'manifest.json').write_text(json.dumps({'schema':1,'source_sha256':sha,'sha256':hashlib.sha256((a.output/'yunet.onnx').read_bytes()).hexdigest(),'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'preparation':'BGR raw0..255 NCHW; right/bottom zero pad to32; no normalization'},indent=2))
