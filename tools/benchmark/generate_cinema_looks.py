"""Original, versioned SDR creative looks. No third-party LUT or film pixels used.

Work in encoded sRGB/Rec709, then bake 33^3 RGB nodes (red fastest).
These are palette interpretations, not production LUTs or film-stock simulation.
Never regenerate or change the frozen legacy Warm Film v1.
"""
from pathlib import Path
import argparse
import colorsys
import hashlib
import json
import math


def spec(key, title, *, contrast=1.12, saturation=0.9, shadow=(0, 0, 0),
         mid=(0, 0, 0), light=(0, 0, 0), black=0.0, white=1.0,
         chroma_boost=0.0, blue_desaturation=0.0, inspiration=None):
    return dict(key=key, title=title, contrast=contrast, saturation=saturation,
                shadow=shadow, mid=mid, light=light, black=black, white=white,
                chroma_boost=chroma_boost, blue_desaturation=blue_desaturation,
                inspiration=inspiration)


# Art direction: each pair affects smooth tonal zones, not segmentation or objects.
LOOKS = [
    spec('matrix-green', 'Matrix - Green Code', contrast=1.18, saturation=.70,
         shadow=(-.035,.034,-.012), mid=(-.030,.024,-.025), light=(-.014,.016,-.023), inspiration='The Matrix (1999)'),
    spec('mad-max-desert', 'Mad Max - Desert Fire', contrast=1.30, saturation=1.20,
         shadow=(-.028,.012,.038), mid=(.032,.006,-.025), light=(.065,.026,-.065), chroma_boost=.12, inspiration='Mad Max: Fury Road (2015), daylight desert'),
    spec('blade-runner-neon', 'Blade Runner 2049 - Neon', contrast=1.22, saturation=1.10,
         shadow=(-.040,.012,.048), mid=(.018,-.022,.025), light=(.058,.020,-.044), chroma_boost=.15, inspiration='Blade Runner 2049 (2017), neon-city interpretation'),
    spec('joker-gotham', 'Joker - Gotham', contrast=1.17, saturation=.78,
         shadow=(-.043,.032,.020), mid=(-.010,.009,-.006), light=(.048,.034,-.035), inspiration='Joker (2019), cyan/sodium urban light'),
    spec('dune-arrakis', 'Dune - Arrakis', contrast=1.09, saturation=.58,
         shadow=(-.004,.004,.008), mid=(.015,.007,-.016), light=(.057,.029,-.045), black=.008, white=.98, blue_desaturation=.45, inspiration='Dune: Part One (2021), Arrakis daylight'),
    spec('dark-knight-steel', 'The Dark Knight - Steel', contrast=1.25, saturation=.68,
         shadow=(-.043,.008,.055), mid=(-.021,.001,.028), light=(-.014,.004,.023), white=.99, inspiration='The Dark Knight (2008), cool urban interpretation'),
    spec('teal-orange', 'Blockbuster Teal & Orange', contrast=1.16, saturation=1.02,
         shadow=(-.048,.027,.038), mid=(.012,.002,-.008), light=(.056,.020,-.050)),
    spec('neon-pink-cyan', 'Cyberpunk Neon Pink & Cyan', contrast=1.20, saturation=1.10,
         shadow=(-.050,.033,.060), mid=(.038,-.045,.033), light=(.055,-.033,.025), chroma_boost=.18),
    spec('purple-gold', 'Magic Purple & Gold', contrast=1.15, saturation=.91,
         shadow=(.021,-.043,.067), mid=(.006,-.012,.013), light=(.065,.038,-.062)),
    spec('red-mint', 'Vintage Red & Mint', contrast=1.10, saturation=.82,
         shadow=(-.045,.050,.025), mid=(.018,-.009,-.008), light=(.070,-.018,-.022), black=.012, white=.985),
    spec('blue-yellow', 'Sunlight Blue & Yellow', contrast=1.16, saturation=1.00,
         shadow=(-.035,-.004,.075), mid=(-.006,.004,.009), light=(.054,.045,-.075)),
    spec('olive-dusty-pink', 'Earthy Olive & Dusty Pink', contrast=.93, saturation=.63,
         shadow=(.018,.026,-.041), mid=(.012,.003,-.006), light=(.058,-.020,.022), black=.028, white=.965),
    spec('navy-gold', 'Luxury Navy & Gold', contrast=1.27, saturation=.79,
         shadow=(-.041,-.015,.070), mid=(-.014,-.006,.022), light=(.071,.044,-.066), white=.985),
    spec('emerald-rose-gold', 'Elegance Emerald & Rose Gold', contrast=1.12, saturation=.86,
         shadow=(-.048,.051,.004), mid=(-.005,.009,-.004), light=(.074,.009,-.016)),
    spec('lavender-sage', 'Calm Lavender & Sage', contrast=.90, saturation=.58,
         shadow=(-.020,.040,-.009), mid=(.011,-.005,.015), light=(.046,-.019,.063), black=.035, white=.96),
]


def luma(rgb):
    return sum(x*w for x,w in zip(rgb, (.2126,.7152,.0722)))


def write_frozen(path, payload):
    if path.exists():
        if path.read_bytes() != payload:
            raise RuntimeError(f'Frozen asset differs: {path.name}; create a new version')
        return
    path.write_bytes(payload)


def transform(rgb, s):
    """Smooth, bounded SDR transform; no analysis, masks, grain, or temporal state."""
    y = luma(rgb)
    # Endpoint-preserving sigmoid. Tone is applied by luminance ratio, avoiding
    # independent RGB contrast curves that bend saturated hues unnecessarily.
    p = s['contrast']
    t = y**p / (y**p + (1-y)**p) if 0 < y < 1 else y
    toned = [x*t/y if y > 1e-12 else 0 for x in rgb]
    sat = s['saturation']
    if s['blue_desaturation']:
        h, chroma, _ = colorsys.rgb_to_hsv(*rgb)
        distance = min(abs(h-2/3), 1-abs(h-2/3))
        sat *= 1-s['blue_desaturation']*math.exp(-.5*(distance/.12)**2)*chroma
    toned = [t + sat*(x-t) for x in toned]
    if s['chroma_boost']:
        # Smooth hue-selective chroma enhancement (cyan and magenta), not skin detection.
        h, chroma, _ = colorsys.rgb_to_hsv(*rgb)
        weights = []
        for center in (.52,.86):
            distance = min(abs(h-center), 1-abs(h-center))
            weights.append(math.exp(-.5*(distance/.10)**2))
        boost = 1+s['chroma_boost']*max(weights)*chroma
        toned = [t+boost*(x-t) for x in toned]
    # Shadow/midtone/light tint vectors are zero-luminance offsets. Saturated
    # RGB bounds are compressed uniformly towards Y, rather than hard-clipped.
    weights = ((1-t)**2, 4*t*(1-t), t*t)
    envelope = 4*t*(1-t)
    for tint, weight in zip((s['shadow'],s['mid'],s['light']),weights):
        ty = luma(tint)
        toned = [x + 3.0*envelope*weight*(c-ty) for x,c in zip(toned,tint)]
    chroma_scale = 1.0
    for x in toned:
        if x < 0: chroma_scale = min(chroma_scale, t/(t-x))
        if x > 1: chroma_scale = min(chroma_scale, (1-t)/(x-t))
    bounded = [t+chroma_scale*(x-t) for x in toned]
    return [min(1., max(0., s['black']+(s['white']-s['black'])*x)) for x in bounded]


def main():
    p=argparse.ArgumentParser(); p.add_argument('output',type=Path); a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=True)
    entries=[dict(menu_value=1,key='natural',title='Natural',selectable=True,cube=None),
             dict(menu_value=2,key='warm-film-legacy-v1',title='(-',selectable=False,
                  cube='warm-film-v1.cube',sha256='ee9e15323b80514ab031a97c9c7d1583132dd96d49bfb6a8e9a6cc43077521b5')]
    for index,s in enumerate(LOOKS,3):
        filename=s['key']+'-v1.cube'
        lines=[f'TITLE "FSTR {s["title"]} v1"','LUT_3D_SIZE 33','DOMAIN_MIN 0 0 0','DOMAIN_MAX 1 1 1']
        for b in range(33):
            for g in range(33):
                for r in range(33):
                    lines.append(' '.join(f'{x:.9g}' for x in transform((r/32,g/32,b/32),s)))
        payload=('\n'.join(lines)+'\n').encode('ascii')
        write_frozen(a.output/filename,payload)
        entries.append(dict(menu_value=index,key=s['key']+'-v1',title=s['title'],selectable=True,
                            cube=filename,sha256=hashlib.sha256(payload).hexdigest(),
                            version=1,recipe=s,inspiration=s['inspiration']))
    catalog=dict(schema=1,library='FSTR Cinema & Palettes v1',input_space='encoded sRGB / Rec.709 primaries',
                 output_space='encoded sRGB / Rec.709 primaries',domain=[0,1],
                 outside_domain='Clamp sampling coordinates only; existing base grading branch unchanged',
                 interpolation='tetrahedral',default_amount_percent=100,slider_percent=[0,100],typed_percent=[0,300],
                 ownership='Original FSTR analytical recipes; no third-party LUTs, film frames or studio assets',
                 limitations='Creative interpretations, not official production LUTs or exact film reconstructions. No spatial relighting, skin recognition, grain or HDR look certification.',
                 entries=entries)
    write_frozen(a.output/'cinema-library-v1.json',(json.dumps(catalog,indent=2)+'\n').encode('utf-8'))
    print(f'Generated {len(LOOKS)} original LUTs; Natural is exact bypass; frozen legacy slot retained as separator')


if __name__=='__main__': main()
