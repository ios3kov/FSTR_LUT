# Visible-face occlusion follow-up

Authorized by user continuation after turn-recovery checkpoint4dce5f5. Apply production-engineering reproduce/isolated-candidate/visual-regression workflow and pinned AE Rulesv11.1.0 Development gate. No installed AE, LUT or native changes; models and footage stay outside Git. Prior TR/SF/MVP gates remain open.

## Acceptance before promoting a candidate

- OC01: reproduce lab prop leak and phone/hand cases; compare visible-face retention separately from occluder rejection. Erasing all face pixels cannot pass.
- OC02: use explicit RGB transfer, crop transform, normalization, model/config hashes and deterministic frame-local inference. Fail invalid input or changed weights; never silently return a successful substitute.
- OC03: current-frame visible-face gate can only reduce the existing mask, never paint background or restore a prior mask through an occluder. No recursive history. Inspect boundary effects and default path preservation.
- OC04: run prior23scene cases plus source-motion cases. Review masks/rendered results separately from numeric tests. Known misses, newly lost faces and uncertainty remain explicit. No general hands/identity guarantee from one case.
- OC05: separate final source/evidence review, exact artifacts and local checkpoint. Continuous normal-speed flicker review remains required and unverified until actually observed; decoding and still sheets cannot substitute.

First candidate: modernized FaceOcc visible-face U-Net/ResNet18, HF revision03f229dc75fa14ae480cca9810f983912c2730ad, canonical probability0.5; evaluate before integrating. This is a different checkpoint from original FaceExtraction, not interchangeable published accuracy. Upstream reports MIT; third-party training-resource rights still require review before redistribution. Local research only. References: https://huggingface.co/mertakin/FaceOcc and https://github.com/face3d0725/FaceExtraction .

## Final candidate v2 result — 2026-10-15

**FAIL quality; research only, no AE promotion.** Fixed roll hypotheses -30/0/+30 improve retained tilted-face support over rejected v1. A larger crop factor2 was rejected after seated-face loss. Final factor1.5 uses the maximum of uncalibrated model scores; this may admit false visibility and does not establish occluder accuracy.

OC01: inspected lab prop removal succeeds locally. Manually selected diagnostic ROIs retain0% of prior prop-mask mass,100% forehead and97.7603% cheek. This is not segmentation IoU, a blinded benchmark or general hand/phone accuracy. Phone cases remain ambiguous; inherited absent faces stay absent.

OC02/OC03: PASS within exercised contracts: pinned model, explicit color conversion, deterministic five nonchronological repeats including float64 input, no new mask coverage, unchanged zero-mask background, finite opaque native output and luma error6.86e-8. Strict baseline geometry/fps/transfer and malformed weight/input tests pass. No temporal state is added; gate runs after existing temporal recovery. Hard threshold may introduce boundary switches.

OC04: FAIL. Actual156 source frames and23 known scene cases processed. All13 detail sheets (156 face crops), five scene boards and native-scale whole-frame panels55/130/133/140/142 visually inspected. Frame55 retains only0.73865% of previous mask mass, visibly losing protection; frames34/37/39/42 also lose substantial support. Unchanged16 zero-mask frames conceals the near-total regression55 and must not be reported as improved continuity. V1 and v2 outputs preserved separately.

OC05: source/output/input hashes and five installed bundle hashes verified; MP4 fully decodes156 frames at25fps/6.24s. Four Python suites pass; static code scanner exits0 with no findings. Native source and renderer remain unchanged. Continuous normal-speed perceptual playback, all-frame native-scale visual review and AE runtime are NOT RUN. No Fast/1080p throughput claim; gate alone measured median210ms,p95468ms on this Mac. Model redistribution clearance remains open.

Next experiment: current-frame motion-compensated evidence from explicit bounded neighbors, with bidirectional agreement and current occluder rejection. Directional boundary softening must be conditional on measured blur; do not blindly blur the whole mask or restore prior face coverage through a hand/object. User suggested motion-aligned blur; this follow-up is not implemented or validated here.

Private artifacts: `outputs/FSTR-LUT-Occlusion-v2`, `work/smart-occlusion-2026-10-15/trial-v2`. Exact report: [OCCLUSION_EVIDENCE.json](OCCLUSION_EVIDENCE.json). Earlier MVP/TR/SF requirements remain open.
