"""Research only. Export official KAIR trunk and verify complete upstream parity."""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path
import numpy as np
import torch
import onnx
import onnxruntime as ort

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--weights", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    revision=subprocess.check_output(["git","-C",str(args.source),"rev-parse","HEAD"],text=True).strip()
    if revision!="fc1732f4a4514e42ce15e5b3a1e18c828af47a1e":
        raise RuntimeError("Unexpected KAIR code revision")
    if hashlib.sha256(args.weights.read_bytes()).hexdigest()!="99a5081e32afdaa25df3ded11aa16447556692e550906caed320db31324af5ce":
        raise RuntimeError("Unexpected research checkpoint bytes")
    # Explicitly reviewed KAIR source checkout; no arbitrary pickle execution.
    sys.path.insert(0, str(args.source.resolve()))
    from models.network_ffdnet import FFDNet
    model = FFDNet(in_nc=3, out_nc=3, nc=96, nb=12, act_mode="R").eval()
    checkpoint = torch.load(args.weights, map_location="cpu", weights_only=True)
    model.load_state_dict(checkpoint, strict=True)
    for parameter in model.parameters():
        parameter.requires_grad_(False)
    args.output.mkdir(parents=True, exist_ok=True)
    graph = args.output / "ffdnet_color_clip_trunk_fp32.onnx"
    torch.onnx.export(model.model, torch.zeros(1,13,128,128), str(graph),
                      input_names=["packed_rgb_sigma"], output_names=["restored_planes"],
                      dynamic_axes={"packed_rgb_sigma":{2:"half_height",3:"half_width"},
                                    "restored_planes":{2:"half_height",3:"half_width"}},
                      opset_version=17, dynamo=False)
    onnx.checker.check_model(str(graph))
    so = ort.SessionOptions(); so.intra_op_num_threads=4;so.inter_op_num_threads=1
    session = ort.InferenceSession(str(graph),so,providers=["CPUExecutionProvider"])
    torch.set_num_threads(4)
    rng=np.random.default_rng(7321)
    comparisons=[]
    with torch.inference_mode():
        for height,width in [(1,1),(7,9),(32,32),(255,257)]:
            image=rng.random((1,3,height,width),dtype=np.float32)
            sigma=torch.full((1,1,1,1),15/255)
            x=torch.from_numpy(image)
            expected=model(x,sigma).numpy()
            padded=torch.nn.functional.pad(x,(0,width%2,0,height%2),mode="replicate")
            shuffled=torch.nn.functional.pixel_unshuffle(padded,2)
            packed=torch.cat([shuffled,sigma.expand(1,1,shuffled.shape[2],shuffled.shape[3])],1)
            planes=session.run(None,{"packed_rgb_sigma":packed.numpy()})[0]
            actual=torch.nn.functional.pixel_shuffle(torch.from_numpy(planes),2).numpy()[...,:height,:width]
            error=float(np.max(np.abs(expected-actual)))
            if error>1e-5: raise RuntimeError(f"Export parity failed {height}x{width}: {error}")
            comparisons.append({"width":width,"height":height,"max_abs_error":error})
        # Odd RGB fixture for C++ comparison, without display quantization.
        width,height=257,255
        image=rng.random((1,3,height,width),dtype=np.float32)
        actual=model(torch.from_numpy(image),torch.full((1,1,1,1),15/255)).numpy()[0].transpose(1,2,0)
        rgba=np.ones((height,width,4),dtype=np.float32);rgba[:,:,:3]=image[0].transpose(1,2,0)
        rgba.tofile(args.output/"parity_input.rgba32f")
        expected=np.ones_like(rgba);expected[:,:,:3]=actual
        expected.tofile(args.output/"parity_expected.rgba32f")
    sha=lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    manifest={"schema":1,"architecture":"ffdnet-color-trunk","model_version":"KAIR-v1.0-color-clip",
              "file":graph.name,"sha256":sha(graph),"weights_sha256":sha(args.weights),
              "weights_source":"https://github.com/cszn/KAIR/releases/download/v1.0/ffdnet_color_clip.pth",
              "code_source":"https://github.com/cszn/KAIR","code_commit":"fc1732f4a4514e42ce15e5b3a1e18c828af47a1e",
              "input_space":"sRGB","normalization":"identity-0-1","layout":"NCHW",
              "input_channels":13,"output_channels":12,"output_scale":1,"temporal_radius":0,
              "precision":"FP32","padding":"replicate-bottom-right-to-even","shuffle_order":"c*4+dy*2+dx",
              "max_pixels":2097152,"sigma_units":"encoded RGB value, noise stddev/255",
              "code_license":"MIT","weights_redistribution":"NOT ESTABLISHED; research only",
              "parameter_count":sum(p.numel() for p in model.parameters())}
    (args.output/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    (args.output/"export-parity.json").write_text(json.dumps({"torch":torch.__version__,"onnx":onnx.__version__,
              "ort":ort.__version__,"seed":7321,"tolerance":1e-5,"comparisons":comparisons},indent=2)+"\n")
    print(json.dumps({"graph_sha256":sha(graph),"comparisons":comparisons,"parameters":manifest["parameter_count"]}))
if __name__=="__main__":
    main()
