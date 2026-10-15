"""Independent existing-corpus checks. No training or inferred ground truth."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from smart_analysis import SkinSegmenter,estimate_balance

def main():
    p=argparse.ArgumentParser()
    for name in ('corpus','model','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    os.environ.setdefault('MPLCONFIGDIR',str(a.output/'mpl'))
    manifest=json.loads((a.corpus/'manifest.json').read_text())
    if manifest['output_contract']!='encoded sRGB/Rec709 primaries RGB8':raise ValueError('Unsupported corpus color contract')
    cases=[]
    for clip in manifest['clips']:
        entry=clip['frames'][0];f=a.corpus/entry['file']
        if hashlib.sha256(f.read_bytes()).hexdigest()!=entry['sha256']:raise ValueError('Corpus bytes changed')
        im=Image.open(f).convert('RGB')
        cases.append((clip['name'],np.asarray(im).astype(np.float32)/255,entry))
    for name,color in [('uniform_beige',[.65,.48,.35]),('night_blue',[.01,.02,.06]),('neutral_grey',[.4,.4,.4])]:
        cases.append((name,np.full((320,768,3),color,np.float32),dict(source='authored uniform color; no person')))
    board=Image.new('RGB',(1152,len(cases)*192+60),(19,21,25));draw=ImageDraw.Draw(board)
    font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',16)
    model=SkinSegmenter(a.model);records=[]
    try:
        for row,(name,rgb,source) in enumerate(cases):
            mask,detail=model.analyze(rgb);balance,bd=estimate_balance(rgb)
            overlay=rgb*(1-mask[...,None]*.65)+np.array([.15,.95,.5])*mask[...,None]*.65
            panels=[rgb,overlay,np.repeat(mask[...,None],3,-1)]
            for col,image in enumerate(panels):
                draw.text((col*384+10,row*192+6),name+' / '+['source','semantic mask','mask weight'][col],font=font,fill='white')
                im=Image.fromarray(np.rint(np.clip(image,0,1)*255).astype(np.uint8)).resize((384,160),Image.Resampling.LANCZOS)
                board.paste(im,(col*384,row*192+32))
            records.append(dict(name=name,source=source,segmentation=detail,balance=balance.tolist(),balance_diagnostics=bd,
                                protected_area_above_half=float((mask>.5).mean()),peak=float(mask.max())))
            print(name,'mask_area',float(mask.mean()),'balance',balance.tolist(),flush=True)
    finally:model.close()
    draw.text((10,len(cases)*192+10),'Tears of Steel - Blender Foundation / Blender Institute - CC BY 3.0 - technical QA only',font=font,fill='white')
    board.save(a.output/'Independent-scenes.png')
    (a.output/'verification.json').write_text(json.dumps(dict(attribution=manifest['attribution'],source_url=manifest['source_url'],
        rights_source=manifest['rights_source'],cases=records,limitations='Already graded movie; no segmentation ground truth, no skin-tone coverage guarantee. Authored colors are negative controls, not real lighting.'),indent=2)+'\n')

if __name__=='__main__':main()
