"""Freeze selected research checkpoint, export and test only validation before held-out acceptance."""
import argparse,hashlib,json,sys,time
from pathlib import Path
import numpy as np
import torch,onnx,onnxruntime as ort
from PIL import Image,ImageOps
from train_student import ResidualStudent
def main():
    p=argparse.ArgumentParser();p.add_argument("--checkpoint",type=Path,required=True);p.add_argument("--corpus",type=Path,required=True)
    p.add_argument("--output",type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    torch.set_num_threads(4);model=ResidualStudent().eval();model.load_state_dict(torch.load(a.checkpoint,map_location="cpu",weights_only=True),strict=True)
    graph=a.output/"student-dynamic.onnx"
    torch.onnx.export(model,torch.zeros(1,13,64,64),str(graph),input_names=["packed_rgb_sigma"],output_names=["restored_planes"],
                      dynamic_axes={"packed_rgb_sigma":{2:"half_height",3:"half_width"},"restored_planes":{2:"half_height",3:"half_width"}},
                      opset_version=17,dynamo=False)
    onnx.checker.check_model(str(graph));so=ort.SessionOptions();so.intra_op_num_threads=4
    session=ort.InferenceSession(str(graph),so,providers=["CPUExecutionProvider"])
    rng=np.random.default_rng(546);parity=[]
    for hh,hw in [(1,1),(7,9),(128,128)]:
        x=rng.random((1,13,hh,hw),dtype=np.float32);x[:,12]=15/255
        with torch.inference_mode():expected=model(torch.from_numpy(x)).numpy()
        actual=session.run(None,{"packed_rgb_sigma":x})[0];e=float(np.abs(expected-actual).max())
        if e>1e-5:raise RuntimeError("Student export parity")
        parity.append({"half_width":hw,"half_height":hh,"max_abs_error":e})
    manifest=json.loads((Path(__file__).resolve().parents[2]/"assets/models/ffdnet-research-manifest.json").read_text())
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    manifest.update(architecture="ffdnet-residual-student",model_version="bounded-student-24x5",file=graph.name,sha256=sha(graph),
                    weights_sha256=sha(a.checkpoint),weights_source="local bounded recipe",parameter_count=sum(p.numel() for p in model.parameters()),
                    weights_redistribution="NOT ESTABLISHED; teacher provenance retained; research only")
    (a.output/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    corpus=json.loads((a.corpus/"manifest.json").read_text());validation=[]
    device="mps"
    if not torch.backends.mps.is_available():raise RuntimeError("MPS unavailable")
    torch.mps.set_per_process_memory_fraction(.4);model=model.to(device)
    def forward(image,sigma):
        x=torch.from_numpy(image.transpose(2,0,1).copy()).unsqueeze(0).to(device)
        h,w=image.shape[:2];x=torch.nn.functional.pad(x,(0,w%2,0,h%2),mode="replicate")
        packed=torch.nn.functional.pixel_unshuffle(x,2);packed=torch.cat([packed,torch.full((1,1,packed.shape[2],packed.shape[3]),sigma,device=device)],1)
        with torch.inference_mode():out=torch.nn.functional.pixel_shuffle(model(packed),2)[0,:,:h,:w].permute(1,2,0).cpu().numpy()
        torch.mps.synchronize();return out
    image=None
    for clip in corpus["clips"]:
        if clip["split"]!="validation":continue
        path=a.corpus/clip["frames"][0]["file"];image=np.asarray(Image.open(path),np.float32)/255
        noise=np.random.default_rng(813).normal(0,12/255,image.shape);noisy=np.clip(image+noise,0,1).astype(np.float32)
        out=forward(noisy,12/255);clean_out=forward(image,3/255)
        mse=lambda a,b:float(np.mean((a.astype(np.float64)-b)**2))
        validation.append({"clip":clip["name"],"input_psnr_db":-10*np.log10(mse(noisy,image)),"student_psnr_db":-10*np.log10(mse(out,image)),
                           "clean_mean_abs_change":float(np.abs(clean_out-image).mean()),"clean_max_abs_change":float(np.abs(clean_out-image).max())})
    full=np.asarray(ImageOps.pad(Image.fromarray(np.uint8(image*255)),(1920,1080)),np.float32)/255;times=[]
    for i in range(4):
        t=time.perf_counter();out=forward(full,12/255);times.append((time.perf_counter()-t)*1000)
    # Fixed-shape graph for independent CoreML C++ probe, identical weights and operations.
    fixed=onnx.load(graph)
    for value,ch in [(fixed.graph.input[0],13),(fixed.graph.output[0],12)]:
        for d,n in zip(value.type.tensor_type.shape.dim,[1,ch,540,960]):d.ClearField("dim_param");d.dim_value=n
    fp=a.output/"student-1080.onnx";onnx.save(fixed,fp)
    static=dict(manifest);static.update(file=fp.name,sha256=sha(fp),packed_shape=[1,13,540,960])
    (a.output/"manifest-1080.json").write_text(json.dumps(static,indent=2)+"\n")
    (a.output/"validation.json").write_text(json.dumps({"parity":parity,"validation":validation,"full_1080_mps_runs_ms":times,
                     "warm_median_ms":float(np.median(times[1:])),"checkpoint_sha256":sha(a.checkpoint),"test_scenes_used":False,
                     "model_parameters":sum(p.numel() for p in model.parameters())},indent=2)+"\n")
    print(json.dumps({"validation":validation,"warm_1080_ms":float(np.median(times[1:])),"parity":parity}))
if __name__=="__main__":main()
