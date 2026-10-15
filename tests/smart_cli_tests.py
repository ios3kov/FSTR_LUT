"""Actual native executable malformed-stream and exact-bypass tests."""
import hashlib
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

exe=Path(sys.argv[1]).resolve(strict=True)
with tempfile.TemporaryDirectory(prefix='fstr-smart-cli-') as name:
    root=Path(name);src=root/'input';params=root/'params';mask=root/'mask'
    packed=struct.pack('<8f',-.0,.2,1.2,1,-.1,.5,.7,1)
    src.write_bytes(packed);params.write_bytes(struct.pack('<4d',0,1,1,1));mask.write_bytes(struct.pack('<2f',.5,1))
    def run(out,maskpath='none',protection='1'):
        return subprocess.run([str(exe),'2','1',str(src),str(maskpath),str(params),str(out),'none','1',protection],capture_output=True)
    good=root/'good';assert run(good,mask).returncode==0
    assert good.read_bytes()==packed
    assert run(good).returncode!=0 and good.read_bytes()==packed
    assert run(src).returncode!=0 and src.read_bytes()==packed
    for n,bad in enumerate([struct.pack('<f',1),struct.pack('<2f',float('nan'),0),struct.pack('<2f',-1,0)]):
        mask.write_bytes(bad);assert run(root/f'bad-mask-{n}',mask).returncode!=0
    params.write_bytes(struct.pack('<4d',.6,1,1,1));assert run(root/'bad-gain').returncode!=0
    params.write_bytes(struct.pack('<4d',0,1,1,1));src.write_bytes(struct.pack('<4f',.2,.3,.4,1));assert run(root/'short-input').returncode!=0
    src.write_bytes(struct.pack('<8f',.2,.3,.4,.5,.2,.3,.4,1));assert run(root/'alpha').returncode!=0
    print('PASS native stream: exact bypass, preserved existing output/input, invalid mask/gain/length/alpha rejection')
