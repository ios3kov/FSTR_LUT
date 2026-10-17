# Automatic shot analysis comparison

Baseline1c304f8. User authorized comparing existing frame masks, bidirectional optical-flow anchors and SAM2 automatically seeded from existing masks. This is a local standalone quality experiment, not an AE integration or another skin detector.

Acceptance: identical source frames and native look; automatic anchor selection with recorded scores; independent forward/backward passes; finite bounded role mass; refuse unsupported/cut propagation; preserve Off; inspect skin-vs-clothing leakage and boundary trails, not only flicker metrics. Save exact source/model/cache hashes and decoded comparison video. No model claim until actual runtime; no continuous perceptual PASS from snapshots. Preserve installed plugins.

Start with frames140–155 of the existing C2098 sequence containing the verified arm failure. Evaluate the same automatic seeds in both propagation routes. SAM2 predicts object geometry, not material identity. Its skin leakage must be measured before any proposal to use its output as skin. The future architecture is a cancellable shot-level analysis/cache, independent of LUT selection; render never advances hidden tracker state. Unknown long gaps remain ordinary-LUT territory.

## Result — 2026-10-15

Implemented four-way comparison: existing masks; direct or chained neighboring-frame flow; bidirectional SAM2; and semantic + SAM2 + flow hybrid. Flow contributes at most25% to compatible foreground geometry and cannot veto a good SAM mask when its own confidence collapses. Foreground agreement is required: agreement that background is empty does not count as object support. Final role fields remain bounded; clothing and absent-person gates remain. Automatic anchors are one arm at141/153, not manually supplied boxes.

On the common SAM-selected arm interior, baseline mean skin weight spans0.217–0.856; SAM2 spans0.644–0.827. This is a diagnostic on a model-selected region, not a ground-truth accuracy or flicker score. Frame150 rises0.253→0.754. Native image differences are subtle because existing graded output remains the base. Flow alone gives no meaningful recovery, even through neighboring frames; the hybrid is nearly identical to SAM2. No evidence supports claiming that all three outperform the simpler pair on this shot.

The important repair separates weak person evidence from grading strength: confidence becomes a smooth confirmation gate over0.1–0.3, while tracked material strength comes from the two anchors. These thresholds were tuned on this sequence, so this is not independent validation. Skin masks still weaken at151 and edge/motion acceptance remains open.

Actual SAM2.1 Hiera Tiny ran on this Mac via PyTorch2.8.0 MPS: final run60.74seconds for16input frames/two independent directions (29predictions). CPU fallback was enabled, not measured as used. This is offline analysis, not a realtime claim. Original and repeated predictions are not bit-exact: mean per-frame probability MAE0.001575, worst0.006583; the cause and operational impact remain unqualified. Cache hashes pin the exact evaluated run.

Model revision `de431c4043854a71d8101e17995dfe596bf101a5`; official code `2b90b9f5ceec907a1c18123530e92e794ad901a4`. Pinned official README declares checkpoint/code Apache2.0; no weights/vendor code are shipped. Model/source/cache hashes are in [evidence](SMART_VIDEO_ANALYSIS_EVIDENCE.json). Input frame payloads are PNG bytes under JPEG filenames for the official directory loader; no lossy JPEG preprocessing occurred.

## Acceptance and limits

- PASS:17 unit contracts; source/model/prediction hash binding; independent directions; automatic anchors; finite bounded masks;64native renders with exact16frame baseline; video decoding; original installed plugin hashes preserved.
- PASS only for snapshots: arm recovery on inspected frames; no obvious new black-clothing region in the inspected outputs. Existing-label clothing metric is a proxy and its veto makes zero added skin unsurprising, not proof of segmentation accuracy.
- NOT RUN: full-video independent quality, crowds/identity swaps, real occlusion/cut corpus, all material roles, continuous perceptual approval and this path in AE. Similar-palette cuts can evade the coarse detector. Long-gap production fallback remains the prior implementation; this comparison falls back to the prior Smart baseline outside its supported interval, not ordinary LUT.
- Repeatability is not qualified. Full-shot cache lifecycle, cancellation orchestration and native backend are not implemented by these research runners.

The first direct-flow run falsely treated camera motion as27cuts; retained outside repo. Reviewed cut veto rejects0on this known single-shot segment. This does not qualify general cut detection.

## Reproduce and next boundary

Use existing external research Python/dependencies. `video_anchor_prepare.py` creates the immutable trial manifest/seeds. `video_sam2_trial.py --device mps --output sam2-final` runs independent directions with exact checkpoint verification. `video_flow_trial.py --strategy neighbor --output flow-neighbor` creates chained flow. `video_anchor_compare.py --flow flow-neighbor --output comparison-reviewed` validates caches and renders all four routes. `video_anchor_report.py` computes the common-region diagnostics. Each output directory must be fresh. See each runner's help for positional paths. Evidence records external absolute artifact locations; footage remains private.

Next: qualify repeatability and test automatic anchor association on separate clips with occlusion, several people and cuts; then test soft edges and continuous motion. Do not port this tuned single-arm path into AE as a universal whole-scene solution.


## Reliability follow-up — 2026-10-17

Added endpoint roundtrip checks: both propagated masks must overlap the independently selected opposite anchor (IoU≥0.35 and anchor coverage≥0.5). On this trial SAM passes (IoU0.846/0.770), neighboring flow fails (0.291/0.313). Rejected flow now contributes exactly zero; its native result equals baseline and hybrid equals accepted SAM on all16frames. Endpoint overlap is a rejection check, not proof of identity in crowds. Unit controls cover wrong object, oversized whole-frame mask, one-sided failure and incompatible occlusion predictions. Real crowd/cut/occlusion qualification remains NOT RUN.

Two fresh independent MPS runs record seed0, strict deterministic-algorithm setting, evaluation mode, identical loaded-state digest, code/config hashes and backend environment. They still differ: mean probability MAE0.000158 over29predictions, versus0.001575 before fixed settings. These are two comparisons, not proof that seed alone caused the reduction. The underlying nondeterministic operation has not been isolated. Worst binary-threshold disagreement184pixels includes predictions outside the accepted interval.

Actual native final-color comparison across16frames: average encoded-sRGB channel error2.54e-7; maximum0.00902 (about2.3/255); at most12pixels/frame exceed1/255. Small measured effect does not establish perceptual temporal acceptance. Cache exact evaluated masks; do not silently re-analyze a shot when changing the LUT. A production cache lifecycle is still future work.

PASS:21anchor/geometry contracts and5real-CLI refusal checks. Reporting rejects empty repeat inventories and mismatched ROI/model caches.128additional native renders pass baseline/finite-alpha checks. Source/data/config/output bindings and measured differences are in [follow-up evidence](SMART_VIDEO_RELIABILITY_EVIDENCE.json). No AE or installed-plugin changes in this follow-up.

Next boundary: independent real footage with several people, occlusion and cuts; soft-edge/motion acceptance; then native cache/backend design. Single-arm quality and whole-scene product readiness remain separate.
