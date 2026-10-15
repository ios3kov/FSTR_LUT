# Whole-scene Smart v1 — standalone implementation

2026-10-15; development branch, no AE installation/promotion. This is an executed research slice of [the whole-scene contract](SMART_SCENE_GRADING.md), not the completed product.

## Implemented

- `fstr_scene_color`: independent C++17 grading module, no AE/ML/Python includes. Straight linear Rec709 RGBA32F; immutable shared LUT, no history/state. Teal & Orange v1 only. Ordinary LUT applied to every pixel; eight nonnegative semantic weights may add a bounded chroma residual, with omitted mass retaining the ordinary look. The residual responds continuously to brightness, saturation and distance from SDR boundaries. No framewise WB or forced canonical skin color. It preserves linear LUT luminance and alpha; extended pixels retain the ordinary branch, with a smooth fade near boundaries. This is original artistic tuning, not a trained professional-colorist model.
- Exact Off/unknown/neutral-look/strength0 behavior at the pure API boundary. API strength accepts0..3. Native CLI currently renders strength1; it requires new output paths and checks flush/close.
- `scene_analysis.py`: actual automatic ADE150 segmentation, grouped into sky/vegetation/water/built/emissive roles; independent MediaPipe person parts supplies skin/hair/clothing only inside scene-person probability. This includes exposed non-face skin when the model identifies it. Neither person boxes nor skin-colored objects alone become skin.
- Source-guided spatial smoothing of probability weights, not binary labels. Explicit±2 window, bidirectional flow/appearance confidence, cut barriers, order-independent iteration. Current support governs smoothing; only immediate two-sided agreement can bridge absent evidence. One-sided/long unsupported gaps do not borrow weights. Confident non-person regions keep working when person analysis fails.
- `scene_trial.py`: complete automatic input→model→soft masks→C++ LUT/residual→output path with hashes, timing and probability diagnostics. `scene_motion_review.py` measures flow-compensated mask/color changes and encodes a labelled ordinary/Smart comparison.

## Model boundary

SegFormer B0 ADE512, exact revision and SHA256 in [manifest](../assets/models/scene-segmentation-research-manifest.json). Model weights were loaded from safetensors locally, no remote model code. RGB normalization and512×512 processor shape checked independently. This candidate's NVIDIA license limits use to research/evaluation; it is NOT a product dependency. The already-known MediaPipe model is also a quality experiment, not newly certified because it is combined with scene segmentation. No footage was uploaded, weights are outside Git/plugin.

Eight roles are grouping policy, not a claim of perfect scene understanding. ADE labels do not provide skin; scene/person-part evidence can still be wrong. Unmapped objects receive the common look. Emissive probability reserves mass but currently has zero extra residual. There is no instance identity, depth/illumination decomposition, automatic exposure correction or physically based material recovery. Pixel flow gives bounded correspondence; it does not prove object tracking.

## Reproduction

External job directory: task-root `work/scene-smart-2026-10-15`. Keep models, private frames and full artifacts outside repository. Research environment is separate from installed plugin/runtime. The exact downloaded package inventory is in `install.log`; model loader uses local files only. Existing `smart-color-env` runs `scene_skin_probe.py` separately, and `research-env` with isolated `deps` runs scene inference.

1. Prepare opaque sRGB PNGs and source SHA256/rights in `full-cases.json`.
2. Run `scene_skin_probe.py images mediapipe-model output-parts`.
3. Build `fstr_scene_render`; run `scene_trial.py --cases ... --model ... --parts ... --renderer ... --lut assets/looks/teal-orange-v1.cube --output NEW_DIRECTORY`.
4. Run `scene_motion_review.py images trial`; inspect both masks and color, including enlarged faces and continuous playback. A decoded video is not perceptual acceptance.
5. CTest covers all existing suites plus `scene_color_contracts`; Python `tests/scene_analysis_tests.py` covers explicit window invariants without loading a model.

## Acceptance and remaining blockers

The full trial and quantitative results are recorded separately in `SMART_SCENE_EVIDENCE.json`. Reviewed stills show useful vegetation/built/clothing grouping and visible human regions without user input. They also show incomplete skin on the complex-light close-up and class leakage on objects in the rooftop scene. These are semantic quality failures, not hidden by successful rendering. The extra color effect is deliberately restrained; a small numerical temporal error alone is not a compelling or professional-looking grade.

Do not integrate the research model into AE. Next required work is a deployable segmentation backend, repair the recorded semantic errors, and validate stronger-but-natural creative effects across material/light combinations before adding more presets. Whole-scene real-host8/16/32, ROI/scale, Undo/state, and MFR remain NOT RUN. Existing installed face-protection trial retains old behavior. Broader Log/metadata obligations remain in PLAN.
