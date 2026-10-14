"""Full C++ frame QA on known authored flat SDR and synthetic Sony-Log encodings.

Existing graded film is a technical reference, never actual camera-Log ground truth.
"""
import argparse, hashlib, json, subprocess
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def decode(x):return np.where(x<=.04045,x/12.92,((x+.055)/1.055)**2.4)
def encode(x):return np.where(x<=.0031308,x*12.92,1.055*np.maximum(x,0)**(1/2.4)-.055)
def save(path,x):Image.fromarray(np.uint8(np.clip(x,0,1)*255+.5)).save(path)
def rgba(rgb):
    result=np.ones((*rgb.shape[:2],4),np.float32);result[:,:,:3]=rgb;return result
def main():
    p=argparse.ArgumentParser()
    for name in ["binary","corpus","matrices","output","looks"]:p.add_argument("--"+name,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    corpus=json.loads((a.corpus/"manifest.json").read_text());matrices=json.loads(a.matrices.read_text())
    inverse=np.array(matrices["rec709_to_sony"])
    report={"schema":1,"binary_sha256":sha(a.binary),"source_manifest_sha256":sha(a.corpus/"manifest.json"),
            "sony_matrix_reference":matrices,"scope":"Authored flat SDR and synthetically encoded SLog3 full-frame fixtures; actual camera Log, AE and subjective product quality not certified",
            "cases":[]}
    for clip in ["test_face","test_night","test_rooftop"]:
        record=next(x for x in corpus["clips"] if x["name"]==clip)
        frame=record["frames"][0];path=a.corpus/frame["file"]
        if sha(path)!=frame["sha256"]:raise RuntimeError("Changed corpus frame")
        source=np.asarray(Image.open(path).convert("RGB"),np.float64)/255;linear=decode(source)
        y=np.sum(linear*np.array([.2126,.7152,.0722]),axis=2,keepdims=True)
        desaturated=y+(linear-y)/1.15
        coordinate=np.sign(desaturated)*np.log1p(np.abs(desaturated)/.01);pivot=np.log(19)
        flat=encode(np.sign(pivot+(coordinate-pivot)/1.2)*.01*np.expm1(np.abs(pivot+(coordinate-pivot)/1.2)))
        camera=linear@inverse.T
        sony=np.empty_like(camera);mask=camera>=.01125
        sony[mask]=(420+261.5*np.log10((camera[mask]+.01)/.19))/1023
        sony[~mask]=(camera[~mask]*(171.2102946929-95)/.01125+95)/1023
        images={"Reference SDR":source,"Authored flat SDR":flat,"Synthetic Sony S-Log3":sony}
        for name,raw,profile,contrast,saturation,look in [
            ("SDR corrected",flat,"host",1.2,1.15,"none"),
            ("Sony normalized",sony,"sony-slog3-cine",1,1,"none"),
            ("Sony + Warm Film",sony,"sony-slog3-cine",1,1,str(a.looks/"warm-film-v1.cube"))]:
            inf=a.output/f"{clip}-{name.replace(' ','-')}-input.rgba32f";outf=inf.with_name(inf.stem+"-output.rgba32f")
            rgba(raw).tofile(inf);h,w=source.shape[:2]
            command=[str(a.binary.resolve()),str(w),str(h),str(inf),str(outf),"srgb",profile,"0",str(contrast),str(saturation),"0",look,"1","1","straight"]
            run=subprocess.run(command,check=True,capture_output=True,text=True)
            result=np.fromfile(outf,np.float32).reshape(h,w,4);output=result[:,:,:3];images[name]=output
            error=float(np.max(np.abs(output-source))) if look=="none" else None
            if error is not None and error>2e-6:raise RuntimeError("Full-frame color parity failed")
            if not np.all(result[:,:,3]==1) or not np.isfinite(result).all():raise RuntimeError("Alpha/nonfinite output")
            report["cases"].append({"clip":clip,"name":name,"max_abs_error_vs_reference":error,"input_sha256":sha(inf),"output_sha256":sha(outf),"command":command,"exit":run.returncode,"stdout":run.stdout.strip(),"range":[float(output.min()),float(output.max())]})
        w,h=source.shape[1::-1];canvas=Image.new("RGB",(w*2,(h+28)*3),(20,20,23));draw=ImageDraw.Draw(canvas)
        for i,(label,img) in enumerate(images.items()):
            x=i%2*w;y=i//2*(h+28);draw.text((x+12,y+8),label,fill="white");canvas.paste(Image.fromarray(np.uint8(np.clip(img,0,1)*255+.5)),(x,y+28));save(a.output/f"{clip}-{i}.png",img)
        canvas.save(a.output/f"{clip}-comparison.png")
    (a.output/"report.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({"cases":len(report["cases"]),"max_normalization_error":max(x["max_abs_error_vs_reference"] or 0 for x in report["cases"]),"scope":report["scope"]}))
if __name__=="__main__":main()
