# Automatic skin toning — initial delta, superseded in scope

The subsequent user instruction requires Smart treatment for all scene objects. See [SMART_SCENE_GRADING.md](SMART_SCENE_GRADING.md). This skin-only draft is preserved as history; fixed skin gains were not promoted to runtime.

Direct user instruction replaces protection with preset-directed toning. No manual region selection. Preserve prior evidence; it does not validate this candidate.

- ST1: Smart Off is byte-identical to ordinary LUT; Natural neutral, alpha, Mix0, Look0, strength100/300 preserved.
- ST2: Smart On blends a distinct skin chroma recipe with scene LUT luminance; recipes belong to each frozen look key. Never describe this as restoring original skin chroma or official film color.
- ST3: Automatic face/hand evidence confines a soft appearance mask; similar colors elsewhere must not select the whole scene. Dark/light skin and unusual lighting need actual image acceptance; a color heuristic is not semantic segmentation.
- ST4: Small/blurred regions have broad soft edges, no detailed holes; no abrupt source-height threshold. Adjacent evidence may smooth; short bounded recovery retained, complex long unsupported gaps remain plain LUT. No previousFrame state.
- ST5: Execute unit/native/sanitizer tests, inspect rendered masks/color, and separately test new bytes inside AE before installing/promoting. Visual flicker remains an explicit gate, never inferred from build success.

Development order: paired grading operator and fixed recipes; soft color-conditioned masks; real hand detector experiment; focused real-frame/motion evaluation; separate host verification. Full-body exposed skin (neck/arms/legs), glove rejection and perceptual motion acceptance remain requirements until verified, not assumed from hand/face geometry.
