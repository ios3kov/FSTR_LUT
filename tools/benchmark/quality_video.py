"""Frozen, paired quality experiment. Not product model selection or a speed test.

Metrics use unclamped encoded-sRGB float output. Display PNG/MP4 clamps explicitly.
Source is already graded/compressed; SIDD homepage illustration is not held-out.
"""
import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
import torch
from PIL import Image, ImageDraw, ImageFilter
from compare_models import PINS, pin, load_nafnet

WEIGHTS = {
    "ffdnet_color_clip.pth": "99a5081e32afdaa25df3ded11aa16447556692e550906caed320db31324af5ce",
    "NAFNet-SIDD-width32.pth": "89c70e808d1783b6c07911306e106aaf0d4f7f3da8c61078b99ff7f8929a26f4",
    "ircnn_color.pth": "ce92ebb5991cb0bf13b9cf88d1e2ab64f2ef03103fc7af0a4b354641901767eb",
}
SIDD = {
    "noisy.png": "0dc570008c212b7e1bcfa60f85b96df949e560b99c500fb6b81f110dbfb41ff3",
    "reference.png": "37d0147b6104e8103dd7793358a5638162da95a51e167c5f32b1780acb86e5dc",
}
CLIPS = ["test_night", "test_face", "test_action", "test_rooftop"]
CASES = ["source", "gaussian5", "gaussian15", "h264", "blur"]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def read(path):
    return np.asarray(Image.open(path).convert("RGB"), np.float32) / 255

def save(path, image):
    Image.fromarray(np.uint8(np.clip(image, 0, 1) * 255 + .5)).save(path)

def grad(image):
    return (image[:, 1:] - image[:, :-1], image[1:] - image[:-1])

def metrics(reference, output):
    if reference.shape != output.shape or not np.isfinite(output).all():
        raise RuntimeError("Non-finite/mis-sized output: failed experiment, no substitution")
    error = output.astype(np.float64) - reference
    mse = float(np.mean(error ** 2))
    rg, og = grad(reference.astype(np.float64)), grad(output.astype(np.float64))
    gmse = float(sum(np.mean((a-b)**2) for a, b in zip(rg, og)) / 2)
    return {"psnr_db": float(-10*np.log10(max(mse, 1e-12))), "mae": float(np.abs(error).mean()),
            "gradient_mse": gmse, "mean_rgb_bias": error.mean((0, 1)).tolist(),
            "min": float(output.min()), "max": float(output.max()),
            "out_of_range_fraction": float(np.mean((output < 0) | (output > 1)))}

def summary(records):
    result = {}
    for key in ["psnr_db", "mae", "gradient_mse", "out_of_range_fraction"]:
        values = [r[key] for r in records]
        result[key] = {"mean": float(np.mean(values)), "min": float(min(values)), "max": float(max(values))}
    result["max_abs_mean_rgb_bias"] = float(np.max(np.abs([r["mean_rgb_bias"] for r in records])))
    result["range"] = [min(r["min"] for r in records), max(r["max"] for r in records)]
    return result

def ordinary(image):
    # Actual ordinary competitor; its RGB8 round-trip is explicit and identical across cases.
    return np.asarray(Image.fromarray(np.uint8(np.clip(image, 0, 1)*255+.5)).filter(ImageFilter.GaussianBlur(.6)), np.float32)/255

def command(args):
    subprocess.run(args, check=True, stdin=subprocess.DEVNULL)

def make_inputs(source, folder, case, clip_index):
    inputs = folder / "input"; inputs.mkdir(parents=True)
    if case == "h264":
        refs = folder / "reference"; refs.mkdir()
        for i, frame in enumerate(source): save(refs/f"{i+1:03}.png", frame)
        # Explicit Rec709 matrix/limited YUV and sRGB transfer; extra chroma subsampling is part of corruption.
        command(["ffmpeg", "-v", "error", "-framerate", "12", "-i", str(refs/"%03d.png"),
                 "-vf", "scale=in_range=pc:out_range=tv:out_color_matrix=bt709,format=yuv420p",
                 "-c:v", "libx264", "-crf", "34", "-preset", "medium", "-threads", "1",
                 "-color_primaries", "bt709", "-color_trc", "iec61966-2-1", "-colorspace", "bt709",
                 "-color_range", "tv", str(folder/"extra-h264.mp4")])
        command(["ffmpeg", "-v", "error", "-i", str(folder/"extra-h264.mp4"),
                 "-vf", "scale=in_color_matrix=bt709:in_range=tv:out_range=pc,format=rgb24", str(inputs/"%03d.png")])
        degraded = [read(p) for p in sorted(inputs.glob("*.png"))]
        if len(degraded) != len(source): raise RuntimeError("H264 frame count mismatch")
    else:
        degraded = []
        for i, frame in enumerate(source):
            if case.startswith("gaussian"):
                sigma = int(case.removeprefix("gaussian"))/255
                # Independent, fixed noise per final-size frame. Same corrupted data for all methods.
                rng = np.random.default_rng(2026101400 + clip_index*1000 + int(sigma*255)*20 + i)
                value = np.clip(frame + rng.normal(0, sigma, frame.shape), 0, 1).astype(np.float32)
            elif case == "blur": value = ordinary(frame)
            else: value = frame.copy()
            degraded.append(value); save(inputs/f"{i+1:03}.png", value)
    # Preserve exact float inputs: metrics/inference do not use PNG quantization of Gaussian corruptions.
    for i, frame in enumerate(degraded): np.save(inputs/f"{i+1:03}.npy", frame)
    return degraded

def sheet(path, images, roi=None):
    w, h = next(iter(images.values())).shape[1::-1]
    if roi: x, y, w, h = roi
    canvas = Image.new("RGB", (w*2, (h+28)*((len(images)+1)//2)), (22,22,24)); d=ImageDraw.Draw(canvas)
    for i, (name, array) in enumerate(images.items()):
        if roi: array = array[y:y+h, x:x+w]
        col, row = i%2*w, i//2*(h+28)
        d.text((col+8,row+7), name, fill="white")
        canvas.paste(Image.fromarray(np.uint8(np.clip(array,0,1)*255+.5)), (col,row+28))
    canvas.save(path)

def main():
    p=argparse.ArgumentParser()
    for key in ["research", "compact-models", "corpus", "sidd", "output"]:
        p.add_argument("--"+key, type=Path, required=True)
    a=p.parse_args(); a.output.mkdir(parents=True, exist_ok=False)
    for repo, digest in PINS.items(): pin(a.research/repo, digest)
    weights={name:(a.compact_models if name=="ircnn_color.pth" else a.research/"models")/name for name in WEIGHTS}
    for name, path in weights.items():
        if sha(path)!=WEIGHTS[name]: raise RuntimeError("Unpinned weights: "+name)
    for name, digest in SIDD.items():
        if sha(a.sidd/name)!=digest: raise RuntimeError("Changed SIDD illustration")
    corpus=json.loads((a.corpus/"manifest.json").read_text())
    if not torch.backends.mps.is_available(): raise RuntimeError("MPS unavailable, no silent CPU substitute")
    torch.set_num_threads(4); torch.mps.set_per_process_memory_fraction(.4)
    sys.path.insert(0,str((a.research/"KAIR").resolve()))
    from models.network_ffdnet import FFDNet
    from models.network_dncnn import IRCNN
    ffd=FFDNet(in_nc=3,out_nc=3,nc=96,nb=12,act_mode="R").eval()
    ffd.load_state_dict(torch.load(weights["ffdnet_color_clip.pth"],map_location="cpu",weights_only=True),strict=True)
    ircnn={}; bank=torch.load(weights["ircnn_color.pth"],map_location="cpu",weights_only=True)
    for key in ["2", "7"]:
        model=IRCNN(in_nc=3,out_nc=3,nc=64).eval(); model.load_state_dict(bank[key],strict=True); ircnn[key]=model
    naf=load_nafnet(a.research/"NAFNet",weights["NAFNet-SIDD-width32.pth"])
    report={"schema":1,"scope":"Quality-first bounded technical experiment; no production selection or speed claim",
            "device":"MPS/M1 Pro", "torch":torch.__version__, "upstream_pins":PINS, "weight_hashes":WEIGHTS,
            "corpus_manifest_sha256":sha(a.corpus/"manifest.json"), "sidd_hashes":SIDD,
            "sidd_source":"https://abdokamel.github.io/sidd/", "sidd_license":"MIT (dataset and code stated by authors)",
            "sidd_limitation":"One public homepage illustration; NAFNet trained on SIDD, not independent held-out test",
            "units":"Unclamped encoded sRGB float, peak1; no global aggregate score",
            "temporal_error":"mean square of (out_t-out_t-1)-(ref_t-ref_t-1); reported separately across source cuts",
            "display":"PNG clamps/quantizes only; MP4 CRF10 identical for all methods, metrics precede encoding",
            "settings":"FFD/IRCNN matched5 for gaussian5; matched15 for gaussian15; weak5 otherwise, fixed15 stress; SIDD weak5/fixed15 not oracle",
            "cases":[]}
    # Freeze reference-derived still ROI before any output; same for every method/case.
    rois={"test_face":[250,24,320,256], "test_night":[240,48,320,256],
          "test_action":[224,32,320,256], "test_rooftop":[270,32,320,256], "sidd":[550,160,512,384]}
    report["fixed_rois"]=rois
    def infer(model, name, image, sigma):
        x=torch.from_numpy(image.transpose(2,0,1).copy()).unsqueeze(0).to("mps")
        with torch.inference_mode():
            if name.startswith("FFD"): out=model(x,torch.full((1,1,1,1),sigma/255,device="mps"))
            else: out=model(x)
            value=out[0].permute(1,2,0).cpu().numpy()
        torch.mps.synchronize(); return value
    datasets=[]
    for ci, name in enumerate(CLIPS):
        entry=next(c for c in corpus["clips"] if c["name"]==name)
        refs=[]
        for frame in entry["frames"]:
            file=a.corpus/frame["file"]
            if sha(file)!=frame["sha256"]: raise RuntimeError("Corpus hash mismatch")
            refs.append(read(file))
        if len(refs)!=12 or any(f.shape!=(320,768,3) for f in refs): raise RuntimeError("Corpus shape/count mismatch")
        # Heuristic cut marks only avoid mixing shots in a statistic; do not certify a production cut detector.
        cuts=[i for i in range(1,len(refs)) if np.mean(np.abs(refs[i]-refs[i-1]))>.12]
        for case in CASES:
            folder=a.output/name/case; folder.mkdir(parents=True)
            degraded=make_inputs(refs,folder,case,ci)
            datasets.append((name,case,refs,degraded,folder,cuts))
    folder=a.output/"sidd"/"real_noise"; folder.mkdir(parents=True)
    (folder/"input").mkdir(); noisy=read(a.sidd/"noisy.png"); reference=read(a.sidd/"reference.png")
    save(folder/"input/001.png",noisy); np.save(folder/"input/001.npy",noisy)
    datasets.append(("sidd","real_noise",[reference],[noisy],folder,[]))
    for name, case, refs, inputs, folder, cuts in datasets:
        sigma=15 if case=="gaussian15" else 5
        variants=[("Input",None,None), ("Ordinary blur0.6",None,None),
                  ("FFD weak-or-matched",ffd,sigma), ("FFD fixed15",ffd,15),
                  ("IRCNN weak-or-matched",ircnn["7" if sigma==15 else "2"],sigma),
                  ("IRCNN fixed15",ircnn["7"],15), ("NAFNet SIDD32",naf,None)]
        record={"clip":name,"case":case,"frame_count":len(refs),"cut_pair_indices":cuts,"methods":{}}
        x,y,w,h=rois[name]; displayed={"Reference":refs[0]}
        for index,(method,model,param) in enumerate(variants):
            method_dir=folder/f"method-{index}"; method_dir.mkdir()
            if model is not None: model.to("mps")
            frame_records=[]; outputs=[]; prev_error=None; within=[]; across=[]; roi_records=[]
            for i,(reference,input_frame) in enumerate(zip(refs,inputs)):
                if method=="Input": out=input_frame
                elif method.startswith("Ordinary"): out=ordinary(input_frame)
                else: out=infer(model,method,input_frame,param)
                frame_records.append(metrics(reference,out)); roi_records.append(metrics(reference[y:y+h,x:x+w],out[y:y+h,x:x+w]))
                error=out.astype(np.float64)-reference
                if prev_error is not None:
                    (across if i in cuts else within).append(float(np.mean((error-prev_error)**2)))
                prev_error=error
                save(method_dir/f"{i+1:03}.png",out)
                if i==0: np.save(method_dir/"first-raw.npy",out); displayed[method]=out
            if model is not None: model.to("cpu"); torch.mps.empty_cache()
            record["methods"][method]={"summary":summary(frame_records),"roi_summary":summary(roi_records),
                "frames":frame_records,"within_shot_tde_mean":float(np.mean(within)) if within else None,
                "across_cut_tde_mean":float(np.mean(across)) if across else None,"sigma":param,
                "display_folder":str(method_dir.relative_to(a.output))}
        baseline=record["methods"]["Input"]
        for method,value in record["methods"].items():
            value["delta_psnr_mean_vs_input"]=value["summary"]["psnr_db"]["mean"]-baseline["summary"]["psnr_db"]["mean"]
            value["delta_gradient_mse_vs_input"]=value["summary"]["gradient_mse"]["mean"]-baseline["summary"]["gradient_mse"]["mean"]
        sheet(folder/"detail.png",displayed,rois[name]); sheet(folder/"frame.png",displayed)
        report["cases"].append(record)
        (a.output/"quality.json").write_text(json.dumps(report,indent=2)+"\n")
        print(json.dumps({"completed":name+"/"+case,"delta_psnr":{m:round(v["delta_psnr_mean_vs_input"],3) for m,v in record["methods"].items()}}),flush=True)
    print("COMPLETE: quality.json; float metrics, original-size stills; motion preview generation is separate",flush=True)

if __name__=="__main__": main()
