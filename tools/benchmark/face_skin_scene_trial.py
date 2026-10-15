"""Known-error regression corpus; not an independent/blind holdout after tuning."""
import argparse,hashlib,json,os
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
from face_skin_analysis import FaceSkinSegmenter,FACE_SHA,DETECTOR_SHA
from smart_analysis import SkinSegmenter,MODEL_SHA


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser()
    for key in ('corpus','model','face-model','detector-model','output'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args()
    for key in ('corpus','model','face_model','detector_model'):setattr(a,key,getattr(a,key).resolve(strict=True))
    manifest=json.loads((a.corpus/'manifest.json').read_text())
    if manifest['output_contract']!='encoded sRGB/Rec709 primaries RGB8':raise ValueError('Unsupported color contract')
    a.output.mkdir(parents=True,exist_ok=False);os.environ.setdefault('MPLCONFIGDIR',str(a.output/'mpl'))
    snapshot=a.output/'source';snapshot.mkdir()
    source_names=('face_skin_scene_trial.py','face_skin_analysis.py','smart_analysis.py')
    for name in source_names:(snapshot/name).write_bytes((Path(__file__).parent/name).read_bytes())
    sources={name:sha(snapshot/name) for name in source_names}
    cases=[]
    for clip in manifest['clips']:
        entry=clip['frames'][0];path=a.corpus/entry['file']
        if sha(path)!=entry['sha256']:raise ValueError('Corpus bytes changed')
        cases.append((clip['name'],np.asarray(Image.open(path).convert('RGB')).astype(np.float32)/255,entry))
    for name,color in [('uniform_beige',[.65,.48,.35]),('night_blue',[.01,.02,.06]),('neutral_grey',[.4,.4,.4])]:
        cases.append((name,np.full((320,768,3),color,np.float32),dict(source='authored uniform negative control')))
    model=FaceSkinSegmenter(a.model,a.face_model,a.detector_model)
    baseline=SkinSegmenter(a.model)
    board=Image.new('RGB',(1536,len(cases)*192+55),(19,21,25));draw=ImageDraw.Draw(board)
    font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',16);records=[]
    try:
        for row,(name,rgb,source) in enumerate(cases):
            old,old_detail=baseline.analyze(rgb);mask,support,detail=model.analyze_with_support(rgb)
            np.savez_compressed(a.output/(name+'.npz'),rgb=rgb,old_mask=old,mask=mask,support=support)
            overlay=lambda m:rgb*(1-m[...,None]*.7)+np.array([.15,.95,.5])*m[...,None]*.7
            for col,im in enumerate([rgb,overlay(old),overlay(mask),np.repeat(mask[...,None],3,-1)]):
                draw.text((col*384+10,row*192+6),name+' / '+['source','previous skin','confirmed face skin','new weight'][col],font=font,fill='white')
                board.paste(Image.fromarray(np.rint(np.clip(im,0,1)*255).astype(np.uint8)).resize((384,160),Image.Resampling.LANCZOS),(col*384,row*192+32))
            records.append(dict(name=name,source=source,baseline=old_detail,candidate=detail,
                                outside_support_max=float(mask[support==0].max(initial=0))))
            print(name,detail,flush=True)
    finally:
        try:model.close()
        finally:baseline.close()
    draw.text((10,len(cases)*192+10),'Tears of Steel / Blender Foundation - CC BY 3.0. Known-error regression; hands/body excluded.',font=font,fill='white')
    board.save(a.output/'Known-error-comparison.png')
    if any(sha(Path(__file__).parent/name)!=sources[name] for name in source_names):
        raise RuntimeError('Source changed during corpus trial; evidence invalid')
    (a.output/'verification.json').write_text(json.dumps(dict(cases=records,attribution=manifest['attribution'],
        source_url=manifest['source_url'],rights_source=manifest['rights_source'],source_sha256=sources,
        model_sha256=dict(skin=MODEL_SHA,face=FACE_SHA,detector=DETECTOR_SHA),
        limitation='No pixel ground truth. Zero on a visible profile/edge face is missed protection, not a quality success.'),indent=2)+'\n')

if __name__=='__main__':main()
