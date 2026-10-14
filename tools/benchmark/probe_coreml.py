"""Fixed-shape CoreML EP probe, still research tooling, not native plugin."""
import argparse,hashlib,json,time
from pathlib import Path
import numpy as np
import onnx,onnxruntime as ort
from PIL import Image
def main():
    p=argparse.ArgumentParser();p.add_argument("--graph",type=Path,required=True);p.add_argument("--image",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True);p.add_argument("--width",type=int,default=512);p.add_argument("--height",type=int,default=384)
    args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    if args.width not in [512,1920] or args.height not in [384,1080] or args.width*args.height>2097152:
        raise RuntimeError("Unapproved probe dimensions")
    image=np.asarray(Image.open(args.image).convert("RGB").resize((args.width,args.height)),np.float32)/255
    h,w=image.shape[:2];hh,hw=(h+1)//2,(w+1)//2
    x=image.transpose(2,0,1);x=np.pad(x,((0,0),(0,h%2),(0,w%2)),mode="edge")
    packed=x.reshape(3,hh,2,hw,2).transpose(0,2,4,1,3).reshape(12,hh,hw)
    packed=np.concatenate([packed,np.full((1,hh,hw),15/255,np.float32)],axis=0)[None].copy()
    graph=onnx.load(args.graph)
    for value,channels in [(graph.graph.input[0],13),(graph.graph.output[0],12)]:
        for dim,n in zip(value.type.tensor_type.shape.dim,[1,channels,hh,hw]):
            dim.ClearField("dim_param");dim.dim_value=n
    fixed=args.output/"fixed.onnx";onnx.save(graph,fixed);onnx.checker.check_model(str(fixed))
    cpu_options=ort.SessionOptions();cpu_options.intra_op_num_threads=4
    cpu=ort.InferenceSession(str(fixed),cpu_options,providers=["CPUExecutionProvider"])
    reference=cpu.run(None,{"packed_rgb_sigma":packed})[0]
    options=ort.SessionOptions();options.intra_op_num_threads=4;options.enable_profiling=True
    options.profile_file_prefix=str(args.output/"ort-profile")
    options.graph_optimization_level=ort.GraphOptimizationLevel.ORT_DISABLE_ALL
    provider_options={"ModelFormat":"MLProgram","MLComputeUnits":"ALL","RequireStaticInputShapes":"1","EnableOnSubgraphs":"0"}
    if "CoreMLExecutionProvider" not in ort.get_available_providers():raise RuntimeError("CoreML EP absent")
    t=time.perf_counter()
    session=ort.InferenceSession(str(fixed),options,providers=[("CoreMLExecutionProvider",provider_options),"CPUExecutionProvider"])
    session_ms=(time.perf_counter()-t)*1000
    runs=[];result=None
    for i in range(4):
        t=time.perf_counter();result=session.run(None,{"packed_rgb_sigma":packed})[0];runs.append((time.perf_counter()-t)*1000)
    profile=Path(session.end_profiling());events=json.loads(profile.read_text())
    providers=sorted({e.get("args",{}).get("provider") for e in events if e.get("args",{}).get("provider")})
    error=float(np.max(np.abs(reference-result)))
    output=result[0].reshape(3,2,2,hh,hw).transpose(0,3,1,4,2).reshape(3,hh*2,hw*2)[:,:h,:w].transpose(1,2,0)
    Image.fromarray(np.uint8(np.clip(output,0,1)*255+0.5)).save(args.output/"coreml-output.png")
    report={"width":w,"height":h,"fixed_graph_sha256":hashlib.sha256(fixed.read_bytes()).hexdigest(),
            "ort":ort.__version__,"provider_options":provider_options,"profile_providers":providers,
            "session_create_compile_ms":session_ms,"inference_runs_ms":runs,"warm_median_ms":float(np.median(runs[1:])),
            "cpu_max_abs_error":error,"fp32_reference_tolerance":1e-5,"fp32_reference_pass":error<=1e-5,
            "device_placement":"CoreML EP placement only; ANE versus GPU not established",
            "scope":"trunk inference only, excludes color/AE/model preparation","profile_file":profile.name}
    (args.output/"coreml-probe.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report))
if __name__=="__main__":main()
