"""Official lighter model comparison; image labels and measured limits remain explicit."""
import argparse,hashlib,json,sys,time
from pathlib import Path
import numpy as np
import torch
from PIL import Image,ImageDraw,ImageOps
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument("--kair",type=Path,required=True);p.add_argument("--models",type=Path,required=True)
    p.add_argument("--image",type=Path,required=True);p.add_argument("--output",type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    if not torch.backends.mps.is_available():raise RuntimeError("MPS unavailable")
    torch.mps.set_per_process_memory_fraction(.4);torch.set_num_threads(4)
    sys.path.insert(0,str(a.kair.resolve()))
    from models.network_ffdnet import FFDNet
    from models.network_dncnn import IRCNN
    gray=FFDNet(in_nc=1,out_nc=1,nc=64,nb=15,act_mode="R").eval()
    gray.load_state_dict(torch.load(a.models/"ffdnet_gray_clip.pth",map_location="cpu",weights_only=True),strict=True)
    ircnn=IRCNN(in_nc=3,out_nc=3,nc=64).eval()
    ircnn.load_state_dict(torch.load(a.models/"ircnn_color.pth",map_location="cpu",weights_only=True)["7"],strict=True)
    original=np.asarray(Image.open(a.image).convert("RGB"),np.float32)/255
    rng=np.random.default_rng(614);noisy=np.clip(original+rng.normal(0,15/255,original.shape),0,1).astype(np.float32)
    outputs={"Source":original,"Added noise":noisy};report=[]
    coefficients=torch.tensor([.2126,.7152,.0722],device="mps").reshape(1,3,1,1)
    luma_sigma=(15/255)*float(np.sqrt(.2126**2+.7152**2+.0722**2))
    def run(model,name,image):
        t=time.perf_counter();x=torch.from_numpy(image.transpose(2,0,1).copy()).unsqueeze(0).to("mps")
        with torch.inference_mode():
            if name=="FFDNet gray luma":
                y=(x*coefficients).sum(1,keepdim=True);restored=model(y,torch.full((1,1,1,1),luma_sigma,device="mps"))
                result=x+(restored-y) # same delta to RGB, no clipping; chroma noise remains.
            else:result=model(x)
            result=result[0].permute(1,2,0).cpu().numpy()
        torch.mps.synchronize();return result,(time.perf_counter()-t)*1000
    for name,model,weights in [("FFDNet gray luma",gray,"ffdnet_gray_clip.pth"),("IRCNN color sigma15",ircnn,"ircnn_color.pth")]:
        model.to("mps");torch.mps.synchronize()
        output,first=run(model,name,noisy);outputs[name]=output
        full=np.asarray(ImageOps.pad(Image.open(a.image).convert("RGB"),(1920,1080),color=(0,0,0)),np.float32)/255
        times=[]
        for i in range(4):_,elapsed=run(model,name,full);times.append(elapsed)
        mse=float(np.mean((output.astype(np.float64)-original)**2))
        report.append({"name":name,"parameters":sum(p.numel() for p in model.parameters()),"weights_sha256":sha(a.models/weights),
          "512_or_preview_first_ms":first,"full_1080_runs_ms":times,"warm_median_ms":float(np.median(times[1:])),
          "paired_added_noise_psnr_db":-10*np.log10(mse),"output_min":float(output.min()),"output_max":float(output.max()),
          "includes_upload_download":True,"chroma":"unchanged encoded chroma noise for luma-only" if model is gray else "RGB",
          "luma_sigma":luma_sigma if model is gray else None,"limitations":"Fixed sigma, no adaptive/temporal or compression claim",
          "mps_driver_bytes":torch.mps.driver_allocated_memory()})
        model.to("cpu");torch.mps.empty_cache()
    w,h=original.shape[1],original.shape[0]
    sheet=Image.new("RGB",(w*2,(h+30)*2),(18,18,18));draw=ImageDraw.Draw(sheet)
    for i,(name,image) in enumerate(outputs.items()):
        x=i%2*w;y=i//2*(h+30);draw.text((x+10,y+8),name,fill="white")
        sheet.paste(Image.fromarray(np.uint8(np.clip(image,0,1)*255+.5)),(x,y+30))
    sheet.save(a.output/"compact-comparison.png")
    (a.output/"results.json").write_text(json.dumps(report,indent=2)+"\n");print(json.dumps(report))
if __name__=="__main__":main()
