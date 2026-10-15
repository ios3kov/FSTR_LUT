# DeepLab local runtime and quality trial — 2026-10-15

Scope: continue selected occlusion-aware DeepLab candidate, preserve existing color/LUT plugin and all prior acceptance requirements. Model/official config identified in OCCLUSION_MODEL_SEARCH.md. Production-engineering regression workflow applied: actual runtime, explicit inputs, symptom reproduction, independent reviewer, final quality rejection. Exact evidence: [DEEPLAB_EVIDENCE.json](DEEPLAB_EVIDENCE.json).

## Runtime resolved

Built official MMCV2.1.0 extension locally for CPU with two compiler jobs against Torch2.8.0. Only setup selection changed: automatic MPS branch becomes explicit opt-in, default CPU. Operator and model implementation source unchanged. Private setuptools80.9/ninja build tools; all dependencies and compiled artifacts outside plugin/repository. Original environment and installed plugin not modified.

MMSeg1.2.2 reference model instantiated without downloading pretrained backbone. Weights-only checkpoint deserialization, pinned SHA and classes check, strict730-key state match passed. Official inference_model is the executed API. BGRuint8 input becomes normalized RGB exactly once. Independent red-pixel oracle confirms channel order and normalization;1280x720 input becomes512x288 with aspect preserved. Official postprocess restores source geometry. Finite2-channel output smoke passed. Face confidence is softmax(logits)[1], not sigmoid of one logit. Face class is intersected with existing skin support, never treated as standalone skin mask.

Read-only senior source review found no relevant forward-architecture discrepancy against MMSeg0.30 (ResNetV1c/contract dilation/separable ASPP/BN/interpolation). This is not numerical parity of complete old/new installations. Explicit backbone→head→two bilinear resizes→softmax matches official API probabilities exactly on frame55 and extra450s; repeated official inference bit-exact on those two cases. Full original6scale/flip evaluation NOT RUN; this is publisher single-scale baseline.

## Executed trial and correction

Executed32 fixed cases: seven source frames, two planted occluders and23 prior scenes. Two independent paths: whole-image official keep-ratio512, and existing face box1.5 context. Initial crop path accidentally retained256 intermediate size before512 inference; preserved as trial-v1, superseded. Final trial-v2 directly samples source crop into512 and uses exact inverse transform. Both modes rerun across all32 after correction; no thresholds, rolls or per-image crop settings adjusted. Raw probability maps and argmax saved separately from support intersection.

Final64 masks/probability maps finite and bounded; output support never exceeds supplied mask. Of18 cases with nonzero existing support, whole-image mode drops16 entirely; direct-crop mode drops9 entirely. These counts are diagnostics against prior support, not precision/recall or ground-truth face accuracy. In particular genuine visible faces54/55/56/130/133 lose protection in both modes. Planted flat/texture cases also zero, which cannot count as occluder success because true-face controls disappear. Inherited zero support131/132 is not evidence about this model. Larger lab/people/face examples produce nonzero crop output, so this is not a blanket constant-background malfunction.

Visually reviewed frame55, texture counterexample and lab prop scene at saved review size. First two show loss of all protection; lab crop preserves facial selection and removes prop. Other generated boards not claimed visually inspected. Known cases only; real occluder ground truth, full156-frame native rendering, continuous flicker review and AE integration NOT RUN for this rejected candidate. Logged timings are CPU diagnostic inference, not calibrated end-to-end performance; parity probe briefly ran concurrently. No Fast claim.

## Decision

Runtime availability PASS; visible-face preservation FAIL for these weights in the tested single-scale paths. Do not promote to AE or count missing masks as successful occlusion rejection. The next remaining shortlist candidate is SegFace, but it still needs actual local inference and independent occlusion evidence; no promise of better quality.

User follow-up requirements were recorded separately in PLAN.md: non-Sony metadata and broader camera/non-camera color encodings. These are planned, not implemented. Metadata recognition and actual Log/gamut transformation remain separate contracts. Current working plugin and prior obligations preserved.
