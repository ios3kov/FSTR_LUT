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
