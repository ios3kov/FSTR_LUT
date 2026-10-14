"""Bounded same-input research comparison. No production model selection."""
import argparse, ast, hashlib, importlib.util, json, resource, subprocess, sys, time, types
from pathlib import Path
import numpy as np
import torch
from PIL import Image, ImageDraw, ImageFilter

PINS={"KAIR":"fc1732f4a4514e42ce15e5b3a1e18c828af47a1e","NAFNet":"2b4af71ebe098a92a75910c233a3965a3e93ede4"}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def pin(p,expected):
    actual=subprocess.check_output(["git","-C",str(p),"rev-parse","HEAD"],text=True).strip()
    if actual!=expected:raise RuntimeError("Unexpected upstream code revision")

def load_nafnet(root,weights):
    # Load reviewed exact upstream classes without installing unrelated BasicSR dependencies.
    for name in ["basicsr","basicsr.models","basicsr.models.archs"]:
        module=types.ModuleType(name);module.__path__=[];sys.modules[name]=module
    utility=types.ModuleType("basicsr.models.archs.arch_util")
    utility.__dict__.update(torch=torch,nn=torch.nn)
    source=ast.parse((root/"basicsr/models/archs/arch_util.py").read_text())
    selected=ast.Module(body=[n for n in source.body if isinstance(n,ast.ClassDef) and n.name in ["LayerNormFunction","LayerNorm2d"]],type_ignores=[])
    exec(compile(selected,"reviewed-upstream-LayerNorm","exec"),utility.__dict__)
    sys.modules[utility.__name__]=utility
    for name,file in [("basicsr.models.archs.local_arch","local_arch.py"),("fstr_nafnet_reference","NAFNet_arch.py")]:
        spec=importlib.util.spec_from_file_location(name,root/"basicsr/models/archs"/file)
        module=importlib.util.module_from_spec(spec);sys.modules[name]=module;spec.loader.exec_module(module)
    model=module.NAFNet(img_channel=3,width=32,middle_blk_num=12,enc_blk_nums=[2,2,4,8],dec_blk_nums=[2,2,2,2])
    checkpoint=torch.load(weights,map_location="cpu",weights_only=True)
    model.load_state_dict(checkpoint["params"],strict=True)
    return model.eval()

def chart(w=512,h=384):
    y,x=np.mgrid[0:h,0:w]
    rgb=np.stack([0.12+0.7*x/(w-1),0.12+0.65*y/(h-1),0.3+0.2*np.sin(x/17)],-1).astype(np.float32)
    image=Image.fromarray(np.uint8(np.clip(rgb,0,1)*255))
    d=ImageDraw.Draw(image)
    colors=[(188,132,102),(224,189,162),(34,42,76),(16,152,54),(220,30,42),(38,74,210)]
    for i,c in enumerate(colors):d.rectangle((i*w//6,h//2,(i+1)*w//6-2,3*h//4),fill=c)
    for x in range(w//2,w,4):d.line((x,0,x,h//3),fill=(10,10,10),width=1)
    d.text((10,10),"FSTR-LUT 0123456 / fine text",fill=(235,235,235))
    d.text((10,32),"SYNTHETIC CHART / not footage or faces",fill=(10,10,10))
    return np.asarray(image,np.float32)/255

def run(model,name,image,device):
    t=time.perf_counter()
    tensor=torch.from_numpy(image.transpose(2,0,1).copy()).unsqueeze(0).to(device)
    with torch.inference_mode():
        if name=="FFDNet":result=model(tensor,torch.full((1,1,1,1),15/255,device=device))
        else:result=model(tensor)
        result=result[0].permute(1,2,0).cpu().numpy()
    if device=="mps":torch.mps.synchronize()
    return result,(time.perf_counter()-t)*1000

def mse(a,b):return float(np.mean((a.astype(np.float64)-b)**2))
def psnr(a,b):return float(-10*np.log10(max(mse(a,b),1e-20)))
def save(path,image):
    # Quantization/clamp is display-only; metrics use untouched float model output.
    Image.fromarray(np.uint8(np.clip(image,0,1)*255+0.5)).save(path)
def main():
    p=argparse.ArgumentParser()
    p.add_argument("--research",type=Path,required=True);p.add_argument("--output",type=Path,required=True)
    p.add_argument("--device",choices=["cpu","mps"],default="cpu")
    p.add_argument("--full-hd",action="store_true")
    args=p.parse_args();r=args.research;o=args.output;o.mkdir(parents=True,exist_ok=True)
    for repo,expected in PINS.items():pin(r/repo,expected)
    if args.device=="mps":
        if not torch.backends.mps.is_available():raise RuntimeError("MPS unavailable, no CPU substitution")
        torch.mps.set_per_process_memory_fraction(0.4)
    torch.set_num_threads(4)
    sys.path.insert(0,str((r/"KAIR").resolve()))
    from models.network_ffdnet import FFDNet
    ffd=FFDNet(in_nc=3,out_nc=3,nc=96,nb=12,act_mode="R").eval()
    ffd.load_state_dict(torch.load(r/"models/ffdnet_color_clip.pth",map_location="cpu",weights_only=True),strict=True)
    naf=load_nafnet(r/"NAFNet",r/"models/NAFNet-SIDD-width32.pth")
    clean=chart();rng=np.random.default_rng(20261014)
    noisy=np.clip(np.round(np.clip(clean+rng.normal(0,15/255,clean.shape),0,1)*255),0,255).astype(np.float32)/255
    ordinary=np.asarray(Image.fromarray(np.uint8(noisy*255)).filter(ImageFilter.GaussianBlur(0.6)),np.float32)/255
    report={"scope":"authored synthetic, Gaussian noise only; no natural-video certification","seed":20261014,
            "device":args.device,"hardware":"Apple M1 Pro, 16 GiB shared memory","torch":torch.__version__,
            "threads":4,"full_input_output_transfer_included":True,"models":[],
            "baseline":{"input_psnr_db":psnr(clean,noisy),"ordinary_blur_psnr_db":psnr(clean,ordinary)}}
    outputs={"Clean":clean,"Noisy":noisy,"Ordinary":ordinary}
    temporal_rng=np.random.default_rng(20261015)
    temporal_inputs=[np.clip(np.round(np.clip(clean+temporal_rng.normal(0,15/255,clean.shape),0,1)*255),0,255).astype(np.float32)/255 for _ in range(5)]
    for name,model,weights in [("FFDNet",ffd,"ffdnet_color_clip.pth"),("NAFNet",naf,"NAFNet-SIDD-width32.pth")]:
        t=time.perf_counter();model=model.to(args.device)
        if args.device=="mps":torch.mps.synchronize()
        load_ms=(time.perf_counter()-t)*1000
        output,first=run(model,name,noisy,args.device)
        _,warm=run(model,name,noisy,args.device)
        untouched,clean_ms=run(model,name,clean,args.device)
        outputs[name]=output
        np.save(o/f"{name.lower()}-raw.npy",output)
        record={"name":name,"weights_sha256":sha(r/"models"/weights),"parameters":sum(x.numel() for x in model.parameters()),
                "width":512,"height":384,"model_to_device_ms":load_ms,"first_frame_ms":first,"warm_frame_ms":warm,
                "noisy_psnr_db":psnr(clean,output),"clean_psnr_db":psnr(clean,untouched),
                "clean_max_abs_change":float(np.max(np.abs(clean-untouched))),
                "output_min":float(output.min()),"output_max":float(output.max())}
        # Five independent noise samples of a static scene: a bounded synthetic flicker probe, not motion.
        frames=[];t=time.perf_counter()
        for degraded in temporal_inputs:
            restored,_=run(model,name,degraded,args.device);frames.append(restored)
        record["static_noise_temporal_std_mean"]=float(np.std(np.stack(frames),axis=0).mean())
        record["five_frame_elapsed_ms"]=(time.perf_counter()-t)*1000
        for i,image in enumerate(frames):save(o/f"{name}-frame-{i:02}.png",image)
        if args.full_hd:
            full=np.asarray(Image.fromarray(np.uint8(noisy*255)).resize((1920,1080)),np.float32)/255
            try:
                full_output,first_hd=run(model,name,full,args.device)
                full_output,warm_hd=run(model,name,full,args.device)
                record["1080p"]={"first_ms":first_hd,"warm_ms":warm_hd,"fps_from_warm":1000/warm_hd,
                                "finite":bool(np.isfinite(full_output).all())}
            except RuntimeError as e:record["1080p"]={"status":"FAIL","error":str(e)}
        if args.device=="mps":
            record["mps_current_allocated_bytes"]=torch.mps.current_allocated_memory()
            record["mps_driver_allocated_bytes"]=torch.mps.driver_allocated_memory()
        record["process_peak_rss_bytes_cumulative"]=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
        report["models"].append(record)
        if args.device=="mps":model.to("cpu");torch.mps.empty_cache()
    width,height=512,384
    sheet=Image.new("RGB",(width*3, (height+28)*2),(22,22,25));draw=ImageDraw.Draw(sheet)
    for i,(label,image) in enumerate(outputs.items()):
        x=(i%3)*width;y=(i//3)*(height+28)
        draw.text((x+12,y+8),label,fill="white")
        tile=Image.fromarray(np.uint8(np.clip(image,0,1)*255+0.5))
        sheet.paste(tile,(x,y+28));save(o/f"{label.lower()}.png",image)
    sheet.save(o/"comparison.png")
    rgba=np.ones((384,512,4),np.float32);rgba[:,:,:3]=noisy;rgba.tofile(o/"noisy.rgba32f")
    rgba[:,:,:3]=clean;rgba.tofile(o/"clean.rgba32f")
    (o/"comparison.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report))
if __name__=="__main__":main()
