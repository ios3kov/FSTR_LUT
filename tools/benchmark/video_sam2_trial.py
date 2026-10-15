"""Independent forward/backward SAM2 mask propagation from automatic anchors."""
import argparse,json,hashlib,time,resource,os
from pathlib import Path
import numpy as np
import torch
from sam2.build_sam import build_sam2_video_predictor
import sam2
p=argparse.ArgumentParser();p.add_argument('trial',type=Path);p.add_argument('model',type=Path);p.add_argument('--device',choices=['cpu','mps'],default='cpu');p.add_argument('--output',default='sam2');a=p.parse_args()
out=a.trial/a.output;out.mkdir(exist_ok=False);m=json.loads((a.trial/'manifest.json').read_text());sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();provenance=json.loads((a.model.parent/'model-provenance.json').read_text())
if sha(a.model)!=provenance['files'][a.model.name]:raise ValueError('Changed model')
for c in m['cases']:
 if sha(a.trial/'frames'/f'{c["index"]:05}.jpg')!=c['sha256']:raise ValueError('Changed video frame')
for anchor in m['anchors']:
 if sha(a.trial/(anchor['direction']+'-seed.npy'))!=anchor['seed_sha256']:raise ValueError('Changed automatic seed')
torch.set_num_threads(4);start=time.perf_counter()
predictor=build_sam2_video_predictor('configs/sam2.1/sam2.1_hiera_t.yaml',str(a.model),device=a.device,apply_postprocessing=False)
records=[]
with torch.inference_mode():
 for anchor in m['anchors']:
  direction=anchor['direction'];state=predictor.init_state(str((a.trial/'frames').resolve()),offload_video_to_cpu=True,offload_state_to_cpu=True)
  mask=np.load(a.trial/(direction+'-seed.npy'),allow_pickle=False)
  predictor.add_new_mask(state,frame_idx=anchor['index'],obj_id=1,mask=mask)
  begin=time.perf_counter();count=0
  for idx,ids,logits in predictor.propagate_in_video(state,start_frame_idx=anchor['index'],reverse=direction=='backward'):
   if ids!=[1] or tuple(logits.shape)!=(1,1,*mask.shape):raise RuntimeError('Unexpected object/output geometry')
   prob=logits[0,0].sigmoid().cpu().numpy().astype(np.float32)
   if not np.isfinite(prob).all():raise RuntimeError('Nonfinite output')
   np.save(out/f'{direction}-{idx:03}.npy',prob);count+=1;print(direction,idx,flush=True)
  records.append(dict(direction=direction,frames=count,seconds=time.perf_counter()-begin));del state
root=Path(sam2.__file__).parent
(out/'evidence.json').write_text(json.dumps(dict(model=provenance,device=a.device,torch=torch.__version__,runs=records,total_seconds=time.perf_counter()-start,maxrss_native=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,manifest_sha256=sha(a.trial/'manifest.json'),source_sha256={str(x.relative_to(root)):sha(x) for x in root.rglob('*') if x.is_file() and x.suffix in ('.py','.yaml')},postprocessing=False,output_sha256={x.name:sha(x) for x in out.glob('*.npy')},mps_fallback_enabled=os.environ.get('PYTORCH_ENABLE_MPS_FALLBACK')=='1',limitations='One automatically selected skin component; no general multi-object identity solution. SAM2 is a geometry predictor, not a skin classifier.'),indent=2))
