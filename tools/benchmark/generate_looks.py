"""Original analytical research looks; not final artistic acceptance."""
from pathlib import Path
import argparse,hashlib,json
def main():
    p=argparse.ArgumentParser();p.add_argument("output",type=Path);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=True)
    for name in ["Neutral","Warm Film"]:
        lines=[f'TITLE "{name} v1"',"LUT_3D_SIZE 17","DOMAIN_MIN 0 0 0","DOMAIN_MAX 1 1 1"]
        for b in range(17):
            for g in range(17):
                for r in range(17):
                    rgb=[r/16,g/16,b/16]
                    if name=="Warm Film":rgb=[rgb[0]*1.025+0.008,rgb[1]*0.99+0.003,rgb[2]*0.965+0.008]
                    lines.append(" ".join(f"{x:.9g}" for x in rgb))
        cube="\n".join(lines)+"\n";slug=name.lower().replace(" ","-")
        (args.output/f"{slug}-v1.cube").write_text(cube)
        look={"schema":1,"look_id":slug,"version":1,"title":name,"input_space":"sRGB","output_space":"sRGB",
              "recommended_amount":1 if name=="Neutral" else 0.6,"tone":"identity","saturation":1,
              "skin_protection":"not implemented in research slice","cube_embedded":cube,
              "cube_sha256":hashlib.sha256(cube.encode()).hexdigest(),
              "limitations":"Research only; not film emulation; no skin/scene recognition; AE viewing unverified"}
        (args.output/f"{slug}-v1.json").write_text(json.dumps(look,indent=2)+"\n")
if __name__=="__main__":main()
