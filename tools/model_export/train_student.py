"""Bounded existing-architecture residual student experiment, research-only."""
import argparse,hashlib,json,sys,time
from pathlib import Path
import numpy as np
import torch
from PIL import Image
class ResidualStudent(torch.nn.Module):
    def __init__(self):
        super().__init__()
        layers=[]
        channels=[13,24,24,24,24,12]
        for i in range(5):
            layers.append(torch.nn.Conv2d(channels[i],channels[i+1],3,padding=1))
            if i<4:layers.append(torch.nn.ReLU())
        self.model=torch.nn.Sequential(*layers)
        torch.nn.init.zeros_(self.model[-1].weight);torch.nn.init.zeros_(self.model[-1].bias)
    def forward(self,packed):return packed[:,:12]+self.model(packed)
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser();p.add_argument("--kair",type=Path,required=True);p.add_argument("--teacher",type=Path,required=True)
    p.add_argument("--corpus",type=Path,required=True);p.add_argument("--output",type=Path,required=True);p.add_argument("--steps",type=int,default=1200)
    a=p.parse_args()
    if not 1<=a.steps<=1800:raise RuntimeError("Finite training budget exceeded")
    a.output.mkdir(parents=True,exist_ok=False)
    if not torch.backends.mps.is_available():raise RuntimeError("MPS unavailable")
    torch.mps.set_per_process_memory_fraction(.4);torch.set_num_threads(4);torch.manual_seed(7126)
    rng=np.random.default_rng(7126);device="mps"
    manifest=json.loads((a.corpus/"manifest.json").read_text())
    train=[a.corpus/f["file"] for c in manifest["clips"] if c["split"]=="train" for f in c["frames"]]
    validation=[a.corpus/c["frames"][0]["file"] for c in manifest["clips"] if c["split"]=="validation"]
    if not train or len(validation)<2:raise RuntimeError("Missing disjoint training/validation")
    images=[np.asarray(Image.open(f),np.float32)/255 for f in train]
    # Authored geometry supplements the one scene, no test scene used.
    for i in range(12):
        y,x=np.mgrid[:320,:768];rgb=np.stack([x/767,y/319,.5+.3*np.sin(x/(4+i))],-1).astype(np.float32)
        rgb[80:160,300:450]=rng.random(3);images.append(rgb)
    sys.path.insert(0,str(a.kair.resolve()));from models.network_ffdnet import FFDNet
    teacher=FFDNet(in_nc=3,out_nc=3,nc=96,nb=12,act_mode="R").eval()
    teacher.load_state_dict(torch.load(a.teacher,map_location="cpu",weights_only=True),strict=True)
    teacher=teacher.model.to(device);student=ResidualStudent().to(device)
    optimizer=torch.optim.Adam(student.parameters(),lr=.001)
    history=[];best=float("inf");start=time.perf_counter()
    def pack(x,sigma):
        packed=torch.nn.functional.pixel_unshuffle(x,2)
        return torch.cat([packed,sigma.expand(-1,1,packed.shape[2],packed.shape[3])],1)
    with torch.inference_mode():
        test=torch.rand(1,13,48,48,device=device);error=float((student(test)-test[:,:12]).abs().max().cpu())
        if error!=0:raise RuntimeError("Initial identity not exact")
    def evaluate():
        student.eval();scores=[]
        for path in validation:
            clean=np.asarray(Image.open(path),np.float32)/255;h,w=clean.shape[:2]
            clean=clean[(h-128)//2:(h-128)//2+128,(w-128)//2:(w-128)//2+128]
            noise=np.random.default_rng(818).normal(0,12/255,clean.shape)
            noisy=np.clip(clean+noise,0,1).astype(np.float32)
            x=torch.from_numpy(noisy.transpose(2,0,1).copy()).unsqueeze(0).to(device)
            gt=torch.from_numpy(clean.transpose(2,0,1).copy()).unsqueeze(0).to(device)
            with torch.inference_mode():
                out=student(pack(x,torch.full((1,1,1,1),12/255,device=device)))
                restored=torch.nn.functional.pixel_shuffle(out,2)
                scores.append(float((restored-gt).square().mean().cpu()))
        student.train();return float(np.mean(scores))
    for step in range(1,a.steps+1):
        patches=[];sigmas=[];clean_flags=[]
        for b in range(4):
            image=images[int(rng.integers(len(images)))];h,w=image.shape[:2]
            y=int(rng.integers(0,h-96+1));x=int(rng.integers(0,w-96+1))
            patch=image[y:y+96,x:x+96].copy()
            if rng.random()<.5:patch=patch[:,::-1].copy()
            if rng.random()<.5:patch=patch[::-1].copy()
            patches.append(patch.transpose(2,0,1))
            clean_flags.append(b==0)
            sigmas.append(float(rng.choice([0,2,5]))/255 if b==0 else float(rng.choice([3,7,12,20,25]))/255)
        clean=torch.from_numpy(np.stack(patches)).to(device)
        sigma=torch.tensor(sigmas,device=device).reshape(4,1,1,1)
        noise=torch.randn_like(clean)*sigma
        noise[0].zero_() # clean target also with erroneous small nonzero sigma.
        noisy=(clean+noise).clamp(0,1)
        packed=pack(noisy,sigma);target=torch.nn.functional.pixel_unshuffle(clean,2)
        predicted=student(packed)
        with torch.no_grad():teacher_output=teacher(packed[1:])
        supervised=(predicted-target).abs().mean()
        clean_loss=(predicted[0]-target[0]).abs().mean()
        distill=(predicted[1:]-teacher_output).abs().mean()
        loss=supervised+clean_loss+.1*distill
        optimizer.zero_grad(set_to_none=True);loss.backward();optimizer.step()
        if step in [1] or step%100==0 or step==a.steps:
            score=evaluate();torch.mps.synchronize()
            record={"step":step,"loss":float(loss.detach().cpu()),"validation_mse":score,"elapsed_seconds":time.perf_counter()-start}
            history.append(record);print(json.dumps(record),flush=True)
            if score<best:
                best=score;torch.save(student.cpu().state_dict(),a.output/"student.pth");student.to(device)
    student.cpu()
    torch.save(student.state_dict(),a.output/"last-step.pth")
    recipe={"schema":1,"architecture":"ffdnet-residual-student","channels":[13,24,24,24,24,12],"params":sum(p.numel() for p in student.parameters()),
            "steps":a.steps,"seed":7126,"batch":4,"patch":96,"initial_identity_error":error,
            "teacher_source":"KAIR official FFDNet Color Clip","teacher_sha256":sha(a.teacher),
            "corpus_manifest_sha256":sha(a.corpus/"manifest.json"),"training_scene_groups":["day_outdoors","authored_synthetic"],
            "validation_scene_groups":["lab_interiors"],"held_out_scene_groups":["future_exteriors"],
            "checkpoint_selection":"validation MSE only; held-out test never accessed",
            "loss":"L1(source)+L1(clean batch)+0.1*L1(teacher on noisy batches)",
            "license":"research only; teacher-weight rights not established; distillation does not clear provenance",
            "limitations":"Bounded artificial Gaussian experiment; source already compressed; no sensor noise/deblock/face certification",
            "history":history,"selected_sha256":sha(a.output/"student.pth"),"elapsed_seconds":time.perf_counter()-start}
    (a.output/"recipe.json").write_text(json.dumps(recipe,indent=2)+"\n")
    print(json.dumps({"selected_checkpoint":recipe["selected_sha256"],"elapsed_seconds":recipe["elapsed_seconds"]}),flush=True)
if __name__=="__main__":main()
