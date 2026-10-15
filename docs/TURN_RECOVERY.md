# Turn/short-gap recovery experiment

Authorized2026-10-15 by user's «делай» after the three-part proposal: short-gap transport, profile selection, confidence transitions. Baseline559f184. Independent research only; installed AE/LUT/native core unchanged. Existing SF01..05, full visual acceptance and broader MVP obligations remain open. Apply Rulesv11.1.0 Development gate and production-engineering behavior/evidence workflow.

| ID | Observable acceptance |
|---|---|
| TR01 | Explicit radius2 inputs and cut barriers, no recursive recovered seeds or previousFrame; equal output for identical windows in arbitrary request order. |
| TR02 | Recover only1–2-frame gaps when mesh-confirmed endpoints exist on both sides, endpoint distance<=3, current face/semantic support and both motion checks agree. Reject single-sided, long-gap, conflicting/multiple-face, cut, out-of-bounds and semantic-negative cases. |
| TR03 | Profile candidate is separately marked: current detector plus face-skin channel and inner region; never a temporal seed. No claim of mesh eye/lip exclusion or hand-occlusion certainty without evidence. Preserve old path/defaults. |
| TR04 | Compare old / new mesh / profile / recovery / combined on exact known failures, unchanged source/grade/native renderer. Explicit positive masks must improve real face coverage, not merely lower an absence count. Old architecture/beige false positives must remain excluded. |
| TR05 | Inspect all156 frames and every changed transition, plus known-error and unused-frame regression cases; inspect occluder/background separately. Numeric consistency and complete MP4 decoding do not certify motion perception. Continuous normal-speed review remains a distinct required check. |
| TR06 | Actual model hashes/source snapshots, failure cleanup, native core tests as affected, final separate review and exact evidence/commit. No media/model redistribution, installation or release. |

First bounded candidate uses a lower proposal threshold only with independent mesh confirmation. Profile fallback, if admitted by evidence, requires a stricter detector score, face-skin semantic channel and an inward feathered proposal region. A box/ellipse alone must never create a skin mask. Weak profile evidence abstains. Existing threshold0.8 baseline stays reproducible.

Recovery is conservative intersection of directly warped original mesh masks from both sides with current semantic/proposal support. Per-side forward/backward motion consistency, image agreement and local texture checks reject unreliable correspondence. No recovered or fallback mask can seed another recovery. Initial recovery is restricted to a single unambiguous face per relevant frame; multiple-face scenes retain spatial masks but abstain from recovery. This is a bounded research limitation, not general multi-person tracking.

Occluding hands may resemble faces and pass semantic/photometric tests. These checks are hypotheses to validate on actual examples, not a proof of occlusion safety. A failing candidate must be retained as negative evidence, not silently promoted or hidden by a zero mask.

## Measured checkpoint — 2026-10-15

Implementation `695235a7d8e91491e68e5e0967d6313dfd1623f1`. Exact artifacts and inspected coverage: [TURN_RECOVERY_EVIDENCE.json](TURN_RECOVERY_EVIDENCE.json). This is an offline research checkpoint, not an installed AE feature.

The new detector/mesh path restores C2098 frames37–39 and42–44. Frame55 is recovered from mesh seeds54/56; the independent profile branch also selects it. Seven previously absent masks now cover visible faces. The old23 zero-mask frames become16; this is not an accuracy measure, because later frames include actual face departure. Four actual old-default checks and three reverse-order model checks are exact. Native color variants keep zero-mask background exact and linear luminance error below6.86e-8.

Profile fallback additionally selects the rooftop profile and the seated face in the570s corpus still. The23-case comparison contains first/last frames from seven known scenes, three controls and six additional fixed timestamps. The90s sample overlaps a known night scene; this is not six independent new scenes or a representative accuracy benchmark. Inspected balusters/beige/night/grey negatives stay excluded.

**FAIL / not accepted:** visible-face masks still disappear at40/41,131/132,140/141 and146. The existing lab case selects a brain prop overlapping the lower face; this is an inherited occlusion defect, not resolved by recovery. Male train profile and frame-edge lab face remain missed. Skin-colored hands, occlusion, eye/lip exclusion in the profile fallback and general multi-person association are not guaranteed. Admission gates are heuristic and still produce abrupt protection changes when they refuse. Soft per-pixel confidence does not solve temporal appearance/disappearance.

**PASS in tested scope:** three Python behavior suites;5/5 Release and5/5 ASan/UBSan native core suites; bounded radius2/order checks; actual models/source hashes and unchanged old input manifests; all four native ablations; both MP4s completely decode156 frames,25fps,6.24s,1440x712. The new runner's injected decoder-failure path was not executed; earlier failure injection belongs to the old runner. No full host acceptance follows from these checks.

**Visual coverage:** all156 frames inspected in seven reduced whole-frame mask sheets and twenty enlarged detail sheets; ten selected full-frame sheets at360x640 panel resolution (37,40,54,55,56,131,132,140,141,146). All five scene boards inspected. Continuous normal-speed perceptual playback and whole-frame native-scale inspection of every frame are **NOT RUN**. The user's full flicker acceptance remains open.

Stage timings were collected while scene analysis overlapped the run, so they are diagnostics, not speed-comparison or1080p/Fast evidence. Installed b7f44 five-file hashes remain unchanged. No model/media redistribution, AE installation, merge, push or release.

A separate local-crop probe found frame40 but returned weak/ambiguous geometry on131; it is retained outside the accepted candidate. Next bounded work should address profile/occlusion parsing on these exact failures before promoting a backend, then complete actual normal-speed review and host gates. No weakened thresholds or zeroed masks were used to hide the known failure.

| Gate | Result |
|---|---|
| TR01 | PASS explicit radius2 and repeatability in inspected tests |
| TR02 | PASS bounded synthetic admission/refusal and actual55; broader occlusion safety unproved |
| TR03 | PASS separate fallback/default contracts; profile accuracy remains incomplete |
| TR04 | PARTIAL gains; FAIL remaining misses and occluder leak |
| TR05 | PARTIAL static coverage; continuous playback NOT RUN |
| TR06 | PASS local evidence/preservation; no release readiness |
