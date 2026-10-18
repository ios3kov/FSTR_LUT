# Native model boundary — 2026-10-18

Local development only. No Python interpreter/PyTorch is linked or called by the effect. Conversion and independent references use existing external research environments. All weights/dylibs remain outside Git; no distribution claim.

## Current evidence

External evidence root: `../native-smart-20261018/`.

- MediaPipe1.1.1 C runtime discovered at the installed wheel's `tasks/c/libmediapipe.dylib`; only Apple/system frameworks in its dependency list, no Python library. `PartsModel` verifies exact runtime digest, pinned skin/face/pose bytes and six output labels before publishing a model context. IMAGE calls, CPU default, serialized requests, explicit row stride, bounded output copying and C-owned result/image/error release.
- Narrow ABI declarations are checked against the installed wheel's `base_options_c.py`, `vision/image_segmenter.py`, `vision/face_landmarker.py`, `vision/pose_landmarker.py` and `components/containers/landmark_c.py`. arm64 sizes BaseOptions72, segmenter options104, result48, normalized landmark40. Independent live calls through the official Python wrapper establish the boundary; declarations alone do not establish it.
- Two source cases: 3-person Tears of Steel crop and user-footage frame. Native six-class probabilities equal official MediaPipe outputs exactly; native face mesh and body masks also equal their independent wrapper reference exactly. Native YuNet detects the same3faces on the group; largest box difference0.038732analysis px versus OpenCV (predeclared1px bound). User-frame control has no accepted face, one body.
- UperNet convnext-tiny pinned source revision876ffc56b819a829448f7e81e9f8606deef6fb65, local card declares MIT. Fixed512shape ONNX export uses exact floor/ceil pooling bins. Floating reduction order differs: substitution max3.24249e-5logit (initial2e-5experimental check rejected; revised justified1e-4bound); ONNX max1.22070e-4 against original PyTorch,3e-4bound. Initial failed exports preserved.
- Two full native preprocessing/model/role comparisons against PyTorch: maximum role error1.03712e-5, person error7.03335e-6; predeclared5e-5bound. Native preprocessing uses explicit half-pixel bilinear uint8 rounding, unlike original PIL antialias; this changes the analysis path and needs new visual acceptance.
- Native all-model current-frame path generated3individual people, soft mesh geometry, cropped skin evidence, pose/material fields and preset-independent raw samples on the group. Automatic cache repeated RAM and disk without a second inference. Separate automatic AE trial now has34exports, fresh-process reuse and pixel-reference checks; corrected Current bundle adds a four-switch host trial. Exact scope and differences are retained in AUTOMATIC_SMART.md.

## Sources and license scope

Primary algorithm/API sources: [MediaPipe segmentation](https://developers.google.com/edge/mediapipe/solutions/vision/image_segmenter), installed1.1.1 C binding declarations, [OpenCV4.12 YuNet decoding](https://github.com/opencv/opencv/blob/4.12.0/modules/objdetect/src/face_detect.cpp). MediaPipe topology indices and ABI-derived declarations retain Apache2 notices under `third_party/mediapipe-abi/`. Binaries, models, model cards and their full notices must be independently reviewed before distribution. The local experiment does not grant redistribution authority.

## Important algorithm differences from prepared acceptance

The native port currently has an explicit approximate polygon distance field rather than OpenCV raster/distanceTransform, a source-gradient detail estimator rather than the research64px Gaussian crop, and an independently implemented YuNet integer-box NMS/resize path (source-edge parity is not yet qualified). Temporal scalar estimation uses a two-sided local median, current geometry and unambiguous matches; it does not yet reproduce the externally scheduled four-frame anchors or the SAM2/full-shot stage. Missing/ambiguous people are never invented. These are open parity/quality items, not silently accepted replacements for the prepared trial. Keep the automatic backend behind `FSTR_NATIVE_SMART` (defaultOFF) and an isolated trial until affected visual/temporal acceptance is closed.
