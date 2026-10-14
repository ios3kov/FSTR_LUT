"""Explicit technical corpus, never production footage or natural-noise ground truth."""
import argparse,hashlib,json,subprocess
from pathlib import Path
def digest(path):
    h=hashlib.sha256()
    with path.open("rb") as stream:
        for data in iter(lambda:stream.read(1024*1024),b""):h.update(data)
    return h.hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument("--movie",type=Path,required=True);p.add_argument("--output",type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    probe=json.loads(subprocess.check_output(["ffprobe","-v","error","-show_streams","-show_format","-of","json",str(a.movie)],text=True))
    if digest(a.movie)!="328253280a05e6b14a92f1f43d818d9b7d38ab5d3f6d5d98606198fd3daef2ff":
        raise RuntimeError("Research movie differs from first-origin captured artifact")
    clips=[("train_day",30,2,"train","day_outdoors"),("validation_lab",60,1,"validation","lab_interiors"),
           ("validation_people",230,1,"validation","lab_interiors"),
           ("test_night",90,1,"test","future_exteriors"),("test_face",340,1,"test","future_exteriors"),
           ("test_action",480,1,"test","future_exteriors"),("test_rooftop",140,1,"test","future_exteriors")]
    records=[]
    filter="colorspace=iall=bt709:irange=tv:all=bt709:trc=srgb:range=pc:format=yuv444p10,scale=768:320,format=rgb24,fps=12"
    for name,time,duration,split,group in clips:
        folder=a.output/name;folder.mkdir(exist_ok=False)
        command=["ffmpeg","-v","error","-ss",str(time),"-i",str(a.movie),"-t",str(duration),"-an","-vf",filter,str(folder/"%03d.png")]
        subprocess.run(command,check=True)
        frames=sorted(folder.glob("*.png"))
        if len(frames)<12:raise RuntimeError("Missing corpus frames")
        records.append({"name":name,"time_seconds":time,"duration_seconds":duration,"fps":12,"split":split,"scene_group":group,
                        "width":768,"height":320,"frames":[{"file":str(x.relative_to(a.output)),"sha256":digest(x)} for x in frames]})
    # Split entire scene families before patches. Test groups never used for checkpoint selection.
    groups={}
    for record in records:
        g=record["scene_group"]
        if g in groups and groups[g]!=record["split"]:raise RuntimeError("Scene group leakage")
        groups[g]=record["split"]
    manifest={"schema":1,"source":"Tears of Steel (2012), Blender Foundation/Blender Institute",
       "source_url":"https://media.xiph.org/tearsofsteel/tears_of_steel_1080p.webm",
       "source_sha256":digest(a.movie),"source_bytes":a.movie.stat().st_size,
       "rights_source":"https://mango.blender.org/about/","license":"CC BY 3.0",
       "attribution":"Tears of Steel, Blender Foundation / Blender Institute, mango.blender.org, CC BY 3.0",
       "portrait_scope":"Technical QA only; no actor endorsement or advertising",
       "full_credit_scroll_preserved_in_source_movie":True,
       "container_color_tags":"not present; input Rec709 limited SDR is explicit research interpretation, not verified source metadata",
       "conversion":filter,"output_contract":"encoded sRGB/Rec709 primaries RGB8",
       "limitations":"Already VP8 compressed/graded source; not noise-free ground truth. No smartphone sensor-noise claim.",
       "clips":records,"probe":probe}
    (a.output/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    print(json.dumps({"clips":len(records),"frames":sum(len(r["frames"]) for r in records),"source_sha256":manifest["source_sha256"]}))
if __name__=="__main__":main()
