"""Actual native process boundary; no model or NumPy dependency."""
import json,math,struct,subprocess,sys,tempfile
from pathlib import Path
renderer=Path(sys.argv[1]).resolve();lut=Path(sys.argv[2]).resolve()
with tempfile.TemporaryDirectory(prefix='fstr-person-cli-') as d:
 root=Path(d);pixels=[(.065,.032,.019,.6),(.57,.38,.25,1.)]
 source=root/'source';source.write_bytes(b''.join(struct.pack('4f',*p) for p in pixels))
 people=[]
 for i,p in enumerate(pixels):
  sample=root/f'samples{i}';sample.write_bytes(struct.pack('4f',*p[:3],1)*40)
  mask=root/f'mask{i}';mask.write_bytes(struct.pack('2f',*(1. if j==i else 0. for j in range(2))))
  people.append(dict(samples=str(sample),mask=str(mask)))
 base=dict(width=2,height=1,input=str(source),lut=str(lut),strength=1.,people=people)
 def run(name,patch,success=True):
  out=root/name;job=root/(name+'.json');job.write_text(json.dumps({**base,'output':str(out),**patch}));result=subprocess.run([str(renderer),str(job)],capture_output=True)
  assert (result.returncode==0)==success,(result.returncode,result.stderr)
  if not success:assert not out.exists()
  return out
 on=run('on',{});off=run('off',{'enabled':False,'people':[{'mask':'/missing/mask','samples':'/missing/samples'}]});ordinary=run('ordinary',{'people':[]})
 assert off.read_bytes()==ordinary.read_bytes()
 data=list(struct.iter_unpack('4f',on.read_bytes()))
 for a,b in zip(data,pixels):assert a[3]==struct.unpack('f',struct.pack('f',b[3]))[0] and all(math.isfinite(x) for x in a)
 for person in people:Path(person['mask']).write_bytes(struct.pack('2f',.8,.8))
 run('invalid-overlap',{},False)
 print('PASS native multi-person CLI, alpha/finite, Off skips missing analysis, excess ownership rejects before output')
