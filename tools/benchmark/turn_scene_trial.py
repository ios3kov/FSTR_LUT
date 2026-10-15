"""Known-error plus fixed additional movie-frame regression; no training/holdout claim."""
import argparse,json,subprocess,os
from pathlib import Path
import numpy as np,cv2
from PIL import Image,ImageDraw,ImageFont
from turn_recovery_trial import sha
from turn_skin_recovery import TurnSkinSegmenter
from face_skin_analysis import FaceSkinSegmenter


def main():
    p=argparse.ArgumentParser()
    for key in ('corpus','movie','model','face-model','detector-model','output'):p.add_argument('--'+key,type=Path,required=True)
    a=p.parse_args();manifest=json.loads((a.corpus/'manifest.json').read_text())
    if manifest['output_contract']!='encoded sRGB/Rec709 primaries RGB8' or sha(a.movie)!=manifest['source_sha256']:raise ValueError('Changed scene source/contract')
    a.output.mkdir(parents=True,exist_ok=False);os.environ.setdefault('MPLCONFIGDIR',str(a.output/'mpl'));cv2.setNumThreads(1)
    snapshot=a.output/'source';snapshot.mkdir()
    for name in ('turn_scene_trial.py','turn_recovery_trial.py','turn_skin_recovery.py','face_skin_analysis.py','smart_analysis.py'):(snapshot/name).write_bytes((Path(__file__).parent/name).read_bytes())
    hashes={x.name:sha(x) for x in snapshot.iterdir()};cases=[]
    for clip in manifest['clips']:
        for position in (0,-1):
            entry=clip['frames'][position];path=a.corpus/entry['file']
            if sha(path)!=entry['sha256']:raise ValueError('Changed corpus frame')
            cases.append((clip['name']+('_first' if position==0 else '_last'),np.asarray(Image.open(path).convert('RGB')).astype(np.float32)/255,entry))
    for name,color in [('beige',[.65,.48,.35]),('night_blue',[.01,.02,.06]),('grey',[.4,.4,.4])]:cases.append((name,np.full((320,768,3),color,np.float32),dict(source='authored control')))
    # Fixed timestamps selected before viewing/analysis, outside the original
    # seven frame selections. These are extra frames, not a representative benchmark.
    for sec in (90,210,330,450,570,690):
        command=['ffmpeg','-v','error','-ss',str(sec),'-i',str(a.movie),'-an','-vf','colorspace=iall=bt709:irange=tv:all=bt709:trc=srgb:range=pc:format=yuv444p10,scale=768:320,format=rgb24','-frames:v','1','-pix_fmt','rgb24','-f','rawvideo','pipe:1']
        raw=subprocess.run(command,capture_output=True,check=True).stdout
        rgb=np.frombuffer(raw,np.uint8).reshape(320,768,3).astype(np.float32)/255
        cases.append((f'extra_{sec}s',rgb,dict(movie_sha256=manifest['source_sha256'],second=sec)))
    candidate=TurnSkinSegmenter(a.model,a.face_model,a.detector_model);baseline=None;records=[]
    try:
        baseline=FaceSkinSegmenter(a.model,a.face_model,a.detector_model)
        font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',15)
        for start in range(0,len(cases),5):
            selected=cases[start:start+5];board=Image.new('RGB',(1536,len(selected)*188+40),(18,20,24));draw=ImageDraw.Draw(board)
            for row,(name,rgb,source) in enumerate(selected):
                before,_,old=baseline.analyze_with_support(rgb);result=candidate.analyze_turn(rgb);detail=result.pop('detail');mask=np.maximum(result['mesh'],result['profile'])
                np.savez_compressed(a.output/(name+'.npz'),rgb=rgb,old_mask=before,mask=mask,**result)
                overlay=lambda m:rgb*(1-.7*m[...,None])+np.array([.15,.95,.5])*.7*m[...,None]
                for col,(label,im) in enumerate(zip(('source','previous face mask','candidate mask','profile branch only'),(rgb,overlay(before),overlay(mask),np.repeat(result['profile'][...,None],3,-1)))):
                    draw.text((col*384+6,row*188+5),name+' / '+label,font=font,fill='white');board.paste(Image.fromarray(np.rint(np.clip(im,0,1)*255).astype(np.uint8)).resize((384,160),Image.Resampling.LANCZOS),(col*384,row*188+25))
                records.append(dict(name=name,source=source,baseline=old,candidate=detail,profile_pixels=int((result['profile']>0).sum())));print(name,detail,flush=True)
            draw.text((10,len(selected)*188+10),'Tears of Steel / Blender Foundation / CC BY 3.0. Research; no general acceptance.',font=font,fill='white');board.save(a.output/f'Scenes-{start:02d}.png')
    finally:
        try:candidate.close()
        finally:
            if baseline is not None:baseline.close()
    if any(sha(Path(__file__).parent/name)!=v for name,v in hashes.items()):raise RuntimeError('Source changed during trial')
    (a.output/'verification.json').write_text(json.dumps(dict(cases=records,sources=hashes,attribution=manifest['attribution'],rights_source=manifest['rights_source'],models={name:sha(getattr(a,name)) for name in ('model','face_model','detector_model')},artifacts={x.name:sha(x) for x in a.output.glob('*.png')}),indent=2)+'\n')

if __name__=='__main__':main()
