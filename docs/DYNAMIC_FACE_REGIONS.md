# Dynamic face regions — 2026-10-15

## Accepted user direction

Automatic selection only. A small head needs approximate localization, not facial geometry, detailed skin segmentation or mandatory shoulders. Each person has independent evidence; ambiguity must not spread another person's mask. As a subject approaches/recedes, increase/decrease detail smoothly. Preserve existing product behavior; this implementation is Python research, not a new installed AE build.

## Implemented contract

Size is estimated head/face box height in original source pixels, using explicit source_scale_y. It is not full-body height, preview zoom or a claim of true anatomical dimensions. Portrait C2098 analysis720×1280 maps to source2160×3840 with scale3. Caller owns this mapping. Other fixtures use1.

| Nominal size | Processing |
| --- | --- |
| below24px | No separate protection |
|24–64px | Weak, broad soft head-color region, maximum0.35 |
|64–160px | Gradual addition of simplified contours |
|above160px | Increasing detailed confirmed face mask |

Exact transitions: activation24–32; coarse-to-medium64–80; medium-to-detailed128–192. These are initial engineering bands, not universally calibrated perceptual thresholds. Coarse regions use only a strong approximate detection box (score>=0.8); no two-scale agreement, skin inference or mesh requirement. They are spatial regions, not brightness-only segmentation. Mesh is skipped below64source pixels. If all detected proposals are smaller than80, pose inference is skipped as well. This does not prove the detector found every person: a missed large person can then miss pose fallback too.

Coarse support persists through64–80 even when the detail detector fails; it fades rather than disappearing at64. Known hand vetoes remain caps after blending. Coarse mode intentionally does not seek exact hand/object/eye/lip exclusions. It cannot be called semantic skin recognition.

Every frame has its own observations. Unique head-box association supplies bounded neighboring size evidence; current height contributes75%, median neighbor height25%. Only nearest compatible observations withinradius2 and across no cuts contribute. Reversing render order does not change inputs/state. Linear approach/recede ramps are unchanged by symmetric neighbors; isolated size jitter is attenuated. This is size stabilization and local association, not persistent identity recognition. Coarse mask position follows the current detected box; position jitter is not solved by smoothing height. Up to2missing coarse observations can be bridged from independent two-sided observations: unique mutual matches, no cuts, no competing intermediate region, interpolated box and compatible16×16RGB patches (mean absolute error<=0.08). Only source heights below80 qualify. Known endpoint hand/anchor exclusions are carried into the interpolated box and capped after blending. Unconfirmed anchors cannot seed recovery. Recovered regions never become seeds; appearance matching cannot identify same-color occluders.

Face-only confirmed observations operate without shoulders. Body/head support remains available for blurred/turned larger faces. Confirmed detail can transition in the same coordinate box as coarse support. Existing two-anchor blur recovery remains separate; this change does not introduce a calibrated automatic blur classifier for every confirmed face. Large-but-blurred detail quality remains unaccepted.

Existing face proposals are capped at16 and poses at4; up to20 combined observations are bounded. Capacity diagnostics explicitly report possible truncation. All confirmed mesh contributors, including weak ones, participate in overlap rejection so a strong face cannot inherit a weaker neighbor's pixels. This is not acceptance for arbitrary crowds.

## Observable acceptance and final review

- PASS40focused tests: face-only group support and more-than-four synthetic observations; all-confirmed overlap rejection; no mesh/skin/pose invocation for appropriate coarse paths; known-veto preservation; exact repeat; size-band continuity; full analyzer63.9/small_region→64.1/no_mesh transition; approach/recede size symmetry; jitter attenuation and cut isolation.
- PASS9anchor and15adaptive unit regressions, plus direct face-skin, turn-recovery and blur regression scripts. Broad unittest discovery was initially unsuitable because smart_cli_tests requires a binary argument; its failure is retained in the private log. The applicable scripts were rerun through their actual entrypoints. No whole-repository test claim.
- PASS static scanner in scanned scope. This is not runtime or quality certification.
- Actual20case scale matrix uses4existing images rescaled to nominal24/48/96/192/320; not newly captured shots or extra source detail. Weak masks now exist on the blurred48px case. Tiny detections and several blurred enlarged faces still fail. Visual inspection of the final overview shows these gaps; no all-scale quality acceptance.
- Fresh C2098 frames48–59: reverse renders exact; repeated model runs55/48/59 compare metadata and person/skin/hand arrays (not separately all anchor tensors). Final frame55 mask sum104.078 is weak partial protection, not skin recall.
- Actual group control now protects3faces without detected bodies; close-up protection returns too. Final control overview inspected. Skin-texture occluder still receives mean0.313409; lab prop false protection remains visible. These are FAIL semantic occlusion, not accepted tradeoffs for the detailed-face route.
- Flat planted patch abstains on the whole face, so not a successful localized exclusion. Beige control remains zero.

PASS: A27-frame synthetic zoom on one still was run in both directions; final run has no zero-mask dropouts and reverse rendering is exact. The first run exposed two mirrored detector dropouts atscale0.55. A bounded coarse-box recovery was added and retested; this is not a real camera approach, real crowd clip or continuous perceptual verdict.

Independent review identified weak-confirmed-neighbor leakage and a cross-branch discontinuity at64; repaired with regression tests. A later review also caught endpoint-veto loss and acceptance of an unconfirmed gap anchor; both were repaired and covered by dedicated tests. Final parent review distinguishes approximate-region implementation from detailed skin acceptance. Latest exact run names, source hashes and installed-preservation checks are in DYNAMIC_FACE_REGIONS_EVIDENCE.json. Earlier private iterations are superseded diagnostics, not final results.

## Remaining acceptance

Continuous perceptual playback, real multi-person crossings/approach/retreat, detector-scale equivalence, arbitrary crowd coverage and native AE integration/performance remain NOT RUN. Semantic occlusion remains FAIL. No new installation, source-footage upload, model redistribution, push, main modification, merge or release. Prior color, metadata and Log requirements remain open.
