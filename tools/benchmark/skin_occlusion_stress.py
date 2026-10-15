"""Synthetic matched-color occluder regression, NOT a real-hand benchmark."""
import argparse,json,importlib.util,hashlib
from pathlib import Path
import cv2,numpy as np
from blur_mask_recovery import recover_blur

def run(recover,seed,kind):
    rng=np.random.default_rng(seed)
    source=cv2.GaussianBlur(rng.uniform([.48,.32,.24],[.72,.56,.48],(96,96,3)).astype(np.float32),(3,3),0)
    current=cv2.GaussianBlur(source,(5,5),0)
    if kind=='flat':current[38:58,38:58]=np.median(source[38:58,38:58],axis=(0,1))
    if kind=='textured':current[38:58,38:58]=cv2.GaussianBlur(source[5:25,5:25],(5,5),0)
    if kind=='green':current[38:58,38:58]=[0,.9,0]
    support=np.zeros((96,96),np.float32);support[25:70,25:70]=1
    out,info=recover([source,current,source],[support]*3,[support,np.zeros_like(support),support],1,[False,False])
    outside=support.copy();outside[36:60,36:60]=0
    return dict(seed=seed,kind=kind,interior_mean=float(out[40:56,40:56].mean()),outside_mean=float(out[outside>0].mean()),status=info['status']),current,out

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--before',type=Path,required=True);a=p.parse_args();a.output.mkdir(exist_ok=False,parents=True)
    spec=importlib.util.spec_from_file_location('before',a.before);old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
    report=[]
    for seed in [731]+list(range(20)):
        for kind in ('control','flat','textured','green'):
            before,_,_=run(old.recover_blur,seed,kind);after,rgb,mask=run(recover_blur,seed,kind)
            report.append(dict(before=before,after=after))
            if seed==731:np.savez_compressed(a.output/(kind+'.npz'),rgb=rgb,mask=mask)
    (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    (a.output/'provenance.json').write_text(json.dumps({str(x):hashlib.sha256(x.read_bytes()).hexdigest() for x in (a.before,Path(__file__),Path(__file__).parent/'blur_mask_recovery.py')},indent=2))
    for kind in ('control','flat','textured','green'):
        rows=[x for x in report if x['after']['kind']==kind];print(kind,'max interior',max(x['after']['interior_mean'] for x in rows),'min outside',min(x['after']['outside_mean'] for x in rows))
if __name__=='__main__':main()
