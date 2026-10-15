# Native Smart Color Grading toggle — development checkpoint2026-10-15

User request: Off renders the previous plain LUT without face/object analysis; On enables automatic regional protection. Default Off preserves existing projects. This native candidate is implemented and locally exercised, but not installed or accepted for final motion quality.

## Contract and implementation

- SC01: checkbox `Smart Color Grading`, stable disk ID13; existing IDs1–12 preserved. Off default. Native metadata UI uses the updated runtime index and unchanged disk ID12.
- SC02: Off, no LUT, zero Look Amount or zero Mix bypass detection. Default ColorProcessor protection0 follows previous color arithmetic. Full-strength protection removes the LUT chroma delta while retaining its linear Rec709 luminance change; global Mix follows this operation. Alpha retained.
- SC03: On uses local macOS Vision face rectangles, with landmarks only for source heights>=64 and available detail. No Python, model downloads or footage logging inside the effect. Detection failure is an error; successful no-face detection gives ordinary LUT output. Up to16face observations; these are soft face regions, not arbitrary-object recognition. Similar-colored items inside those regions are not semantically excluded.
- SC04: source-height24–32activation and64–192detail transition, soft ellipse and feathered internal exclusions. Maximum mask0.65, protection multiplier0.8. Details remove at most half the base. Current weak detail cannot be raised by sharp neighbors. Detail heuristic is provisional and differs from the Python research implementation.
- SC05: SmartFX requests full input independently of output ROI, plus explicit valid adjacent frames. Compatible two-sided unique observations soften position/size. No persistent previousFrame, no native detection-gap recovery. Whole-frame appearance threshold is a heuristic, not robust scene-cut certification. Legacy render currently has current-frame detection only.
- SC06: square-pixel detector preparation accounts for PAR and asymmetric downsample; normalized coordinates map back to complete input. Local native checks do not establish all real AE scale/PAR/cropped-layer behavior.
- SC07: errors/cancel/checkouts remain balanced, no exception crosses EffectMain. Vision inference serialized to bound simultaneous requests; cancellation checked during preparation/render, not inside Apple's request. No Fast25–30fps claim.

## Executed checks

Release and ASan/UBSan builds: eight CTest targets PASS, including new feather continuity/chroma luminance oracles and owned SmartFX full-input/temporal/error-cleanup fixtures. These load actual module bytes outside AE.

Real native module test on supplied frame48 at540×960 sRGB analysis: Smart On changed563pixels; alpha unchanged; switching Off restored plain LUT output exactly. This exercises actual Apple Vision and plugin processing, not a mocked detector. First sandbox attempt could not run Vision; an explicitly scoped local run outside sandbox succeeded.

Native detector-only scan of156frames at540×960 found at least one face in78frames. This is coverage, not accuracy: the face leaves view near the end, and real visible-face misses occur at54/55/56. Six sampled overlays inspected:48/80/130located head areas;54/55/56empty. Therefore native motion/blur quality is NOT ACCEPTED. The Python prototype's gap recovery is not present here, and its results cannot certify this detector.

Senior read-only review found missing PAR/asymmetric analysis handling and missing SmartFX pre-render/On checks. Both addressed. Final local source/diff, build and tests reviewed separately from visual/host acceptance.

## Remaining acceptance

NOT RUN: actual AE loading/UI toggle, old-project migration/save/reopen/Undo, host MFR/ROI/PAR/alpha scenes, continuous perceptual playback, Windows. Native full Smart behavior is incomplete: no body/head fallback, no short-gap recovery, no arbitrary-object detection; visible-face misses remain. Do not replace installed plugin on these results or describe the full Smart feature as finished.

Next: port a validated bounded coarse head/gap path using current-image evidence, test native detector coverage and actual On/Off motion on the same sequence, then isolated AE trial. Existing installed bundle and all previous failed evidence are preserved. Broader metadata/Log obligations remain in PLAN.md.
