"""Independent forward/backward SAM2 mask propagation from automatic anchors."""
import argparse,json,hashlib,time,resource,os,random
from pathlib import Path
import numpy as np
import torch
from sam2.build_sam import build_sam2_video_predictor
import sam2
p=argparse.ArgumentParser();p.add_argument('trial',type=Path);p.add_argument('model',type=Path);p.add_argument('--device',choices=['cpu','mps'],default='cpu');p.add_argument('--output',default='sam2');p.add_argument('--seed',type=int,default=0);p.add_argument('--deterministic',action='store_true');p.add_argument('--save-objects',action='store_true');a=p.parse_args()
out=a.trial/a.output;out.mkdir(exist_ok=False);m=json.loads((a.trial/'manifest.json').read_text());sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();provenance=json.loads((a.model.parent/'model-provenance.json').read_text())
if sha(a.model)!=provenance['files'][a.model.name]:raise ValueError('Changed model')
for c in m['cases']:
 if sha(a.trial/'frames'/f'{c["index"]:05}.jpg')!=c['sha256']:raise ValueError('Changed video frame')
for anchor in m['anchors']:
 if sha(a.trial/(anchor['direction']+'-seed.npy'))!=anchor['seed_sha256']:raise ValueError('Changed automatic seed')
random.seed(a.seed);np.random.seed(a.seed);torch.manual_seed(a.seed)
torch.use_deterministic_algorithms(a.deterministic)
torch.set_num_threads(4);start=time.perf_counter()
predictor=build_sam2_video_predictor('configs/sam2.1/sam2.1_hiera_t.yaml',str(a.model),device=a.device,apply_postprocessing=False)
if predictor.training:raise RuntimeError('Predictor is not in evaluation mode')
state_digest=hashlib.sha256()
for key,value in sorted(predictor.state_dict().items()):
 state_digest.update(key.encode());state_digest.update(value.detach().cpu().contiguous().numpy().tobytes())
records=[]
with torch.inference_mode():
 for anchor in m['anchors']:
  direction=anchor['direction'];state=predictor.init_state(str((a.trial/'frames').resolve()),offload_video_to_cpu=True,offload_state_to_cpu=True)
  mask=np.load(a.trial/(direction+'-seed.npy'),allow_pickle=False)
  if mask.ndim==2:mask=mask[None]
  if mask.ndim!=3 or not 1<=len(mask)<=16:raise ValueError('Invalid object seeds')
  for obj_id,object_mask in enumerate(mask,1):predictor.add_new_mask(state,frame_idx=anchor['index'],obj_id=obj_id,mask=object_mask)
  begin=time.perf_counter();count=0
  for idx,ids,logits in predictor.propagate_in_video(state,start_frame_idx=anchor['index'],reverse=direction=='backward'):
   if ids!=list(range(1,len(mask)+1)) or tuple(logits.shape)!=(len(mask),1,*mask.shape[1:]):raise RuntimeError('Unexpected object/output geometry')
   probabilities=logits[:,0].sigmoid().cpu().numpy().astype(np.float32)
   prob=probabilities.max(axis=0)
   if a.save_objects:np.save(out/f'{direction}-{idx:03}-objects.npy',probabilities)
   if not np.isfinite(prob).all():raise RuntimeError('Nonfinite output')
   np.save(out/f'{direction}-{idx:03}.npy',prob);count+=1;print(direction,idx,flush=True)
  records.append(dict(direction=direction,frames=count,seconds=time.perf_counter()-begin));del state
root=Path(sam2.__file__).parent
(out/'evidence.json').write_text(json.dumps(dict(per_object_output=a.save_objects,seed=a.seed,deterministic_algorithms=torch.are_deterministic_algorithms_enabled(),loaded_state_sha256=state_digest.hexdigest(),runner_sha256=sha(Path(__file__)),mps_environment={k:os.environ.get(k) for k in ['PYTORCH_ENABLE_MPS_FALLBACK','PYTORCH_MPS_FAST_MATH','PYTORCH_MPS_PREFER_METAL']},model=provenance,device=a.device,torch=torch.__version__,runs=records,total_seconds=time.perf_counter()-start,maxrss_native=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,manifest_sha256=sha(a.trial/'manifest.json'),source_sha256={str(x.relative_to(root)):sha(x) for x in root.rglob('*') if x.is_file() and x.suffix in ('.py','.yaml')},postprocessing=False,output_sha256={x.name:sha(x) for x in out.glob('*.npy')},mps_fallback_enabled=os.environ.get('PYTORCH_ENABLE_MPS_FALLBACK')=='1',limitations='Automatically seeded skin components. Optional per-object outputs retain seed order, not biometric identity. SAM2 is a geometry predictor, not a skin classifier.'),indent=2))
