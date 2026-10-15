# Smart Color — independent quality prototype

User authorization: 2026-10-15 «делай» following the proposed Auto Balance + semantic Skin Protection + video stability experiment. This extends color scope only. Preserve existing Natural, LUT assets, parameter IDs, installed b7f44 build and projects. No restoration, training, generative relighting or release. Rules remain v11.1.0 /04b606845e0f73ab28b9807bb45682eae4d6ce34. Critical numeric/data risk; Development gate.

## Prototype acceptance

| ID | Observable criterion |
|---|---|
| SC01 | Pinned official model, actual inference, verified label order, input transfer, mask dimensions and finite range; unavailable/invalid model fails explicitly. No media upload. |
| SC02 | Inspect supplied clip at multiple times: face/hands vs clothing/walls; expose confidence overlay and failures. Test person crop refinement if full-frame inference loses small faces. One person cannot establish broad skin-tone quality. |
| SC03 | Independent native math: protected look retains declared linear luminance, protection0 equals unprotected LUT, look0 equals prepared base; finite extended values, invalid masks/settings and deterministic calls. Prototype video is opaque SDR only. |
| SC04 | Conservative automatic preparation: explicit SDR decoding; no profile guessing, no recovered illuminant/CCT claim. Bounded exposure/WB, deadbands, neutral-candidate support/distribution/agreement gates; uncertain samples return identity. Test night/neon/flat/neutral/cast and clipping cases. |
| SC05 | Explicit radius2 window stabilizes both masks and balance. Never cross any intermediate cut; current mask dominates combined neighbors; invalid warp/occlusion rejected. Same window yields same result in reverse/random order. No previousFrame. |
| SC06 | Compare original, LUT-only, balance-only and protected look on frames and a short video. Inspect halos/trailing/flicker separately from numeric checks. Record timing without promising AE/Fast performance. |
| SC07 | Promote to AE only after independent quality acceptance. Native runtime, persistence, masks/ROI/scale/MFR and full source-space checks remain mandatory for integration; prototype success is not host success. |

## Architecture and boundaries

### Face-supported follow-up (2026-10-15)

User explicitly requests stronger skin selection including face confirmation, evaluated on known errors, and complete visual flicker inspection. Preserve the previous failed backend/evidence. Candidate is a research-only **face skin** selector: independent YuNet proposals are confirmed by IMAGE-mode face landmarks; semantic skin probabilities refine the confirmed interior. No unconfirmed body/hands fallback and no identity recognition. This narrower candidate does not fulfill whole-body skin protection.

| ID | Follow-up acceptance |
|---|---|
| SF01 | Pin official face model bytes/API/model-card; no media upload or model redistribution. Exact finite geometry, bounded views, no hidden tracking state. |
| SF02 | Compare original and candidate on all previous seven scenes/three controls. Inspect missed side-lit face and false-positive balusters explicitly. Zero no-face result is safe abstention, not successful skin detection. No general skin-tone claim. |
| SF03 | Same 156-frame C2098 window, same decode/LUT/balance/native renderer. Independent face support must not create new background protection; inspect missed detections and mask edges. Actual reverse-order requests repeat. |
| SF04 | Inspect the entire 0..155-frame output, with consecutive-frame detail and every transition rather than sparse 1/2/4s stills. Review at native preview scale and enlarged face detail. Record exact reviewed artifacts, defects/timecodes, coverage and limits. Continuous real-time playback is a separate check if tools cannot expose moving pixels; do not mark it PASS from screenshots or metrics. |
| SF05 | Test absent face, invalid/edge geometry, features excluded from mask, temporal cuts/reveal/occlusion and fresh-output preservation. Re-run affected numeric/CLI checks. No AE integration until quality gates pass. |

Full visual inspection may conclude FAIL; a completed review is not a passing quality gate. Existing SC01..07 and broader MVP obligations remain in force.

Research inference/analysis runs in a separate Python environment outside the delivered plugin. Reusable pointwise look/protection math is C++17 without AE headers. Prototype input is explicitly prepared straight opaque linear Rec709; analysis sees an sRGB display copy. Learned skin confidence is not a calibrated matte or a hue-only skin detector. Model load occurs once per research run; models/footage stay outside Git and plugin packaging.

Order: explicit input decode -> conservative Auto Balance -> original frozen LUT -> selective chroma protection. This prototype does not add new manual grading controls or alter existing plugin defaults. Natural stays exact when Smart Color is off. For future integration, manual grade and shoulder belong after balance, before LUT; global Mix remains last.

Let B be prepared linear RGB and L the LUT result at selected strength. Protection removes the color-difference part of L-B: O=L-p*((L-B)-Y(L-B)*(1,1,1)). This preserves linear Rec709 Y, not perceptual saturation, white balance or chromaticity. Darkened/extreme looks can create negative channels; preserve finite floats in the kernel, count display clipping in research outputs, do not hide it with core clamps. Low-confidence mask fades protection to zero.

Auto Balance is a bounded aesthetic heuristic, not scene-intention recovery. It must not force every median to camera middle gray. White balance requires distributed low-chroma candidates with consistent ratios; limited evidence yields identity. Night/colored-light artistic acceptance is separate from bounded output.

Temporal neighbors are explicit. Flow is computed from pre-look display copies; analysis is independent of selected LUT/strength. Cut detection is a heuristic with known flash/gradual-transition and similar-palette hard-cut ambiguity; synthetic boundary tests supplement real motion. Only supplied/detected boundaries are guaranteed not to be crossed. Identity/uncertain central exposure and WB remain identity even if neighbors suggest a correction. Any integration must expose/cache these dependencies to AE rather than inheriting sequential VIDEO-mode state.

The previous retained candidate uses full-frame inference plus four fixed overlapping semantic views, with feathered confidence fusion; masks retain current-frame dominance. This addresses missed small subjects without moving crop geometry. It does not solve semantic false positives. Earlier full-frame/coarse-person-crop trials are retained as negative evidence outside Git. Body/face weights use smoothstep0.45..0.85 and strength80% in comparisons; thresholds are research choices, not calibrated probabilities.

Observed independent-corpus failure: the model selected architectural balusters in two complex-light scenes and missed much of a visible face in one; a uniform beige negative control also produced a small false positive. The previous candidate's SC02 overall quality is FAIL. That candidate must not be promoted into the installed AE plugin. No denoising/other enhancement has been added to compensate for this.

## Reproduction

Use a separate Python3.12 arm64 environment with `tools/benchmark/smart-requirements-lock-macos-arm64.txt`. Model location is external; its official pinned URL, SHA, label order and actual metadata normalization are in `assets/models/smart-skin-research-manifest.json`. Metadata extraction's high-level Python helper was unavailable in the installed wheel; direct FlatBuffer metadata inspection verified input/output shapes and mean/std127.5. Runtime IMAGE inference itself succeeded.

Build `fstr_smart_render` and run core CTest normally. Run `tests/smart_analysis_tests.py` in the research environment. `tools/benchmark/smart_color_trial.py --help` describes the private opaque SDR portrait/Teal trial. Every trial requires new job/output paths and an explicit input transfer; it snapshots executed Python files. Source preview is360x640/25fps, capped156frames; it is not a full-resolution or AE performance test. Native stream files are same-machine little-endian research data, not a public interchange format. Fresh output is refused if a target already exists; a failed stream may leave an incomplete research file and reports failure.

`smart_scene_trial.py` reuses the hash-checked existing seven-scene Tears of Steel research corpus and three authored negative controls. Film attribution is displayed on its output. Existing split names are retained historical names; this experiment does not train or tune against those scenes. Portrait/still evidence does not establish all skin tones or full video acceptance.

## Sources

- [Official MediaPipe Image Segmenter](https://developers.google.com/edge/mediapipe/solutions/vision/image_segmenter): zero-based 2 body-skin, 3 face-skin, IMAGE mode/confidence output.
- [Official multiclass model card](https://storage.googleapis.com/mediapipe-assets/Model%20Card%20Multiclass%20Segmentation.pdf): Apache2.0 model, no pixel-perfect matte guarantee; low light/backlight/motion/occlusion limitations. Download is pinned by SHA256 in the model manifest; weights are not redistributed in this milestone.

Current progress and exact evidence will be recorded in STATUS.md and SMART_COLOR_EVIDENCE.json. Original MVP/Windows/Log/HDR/AE obligations in PLAN.md remain open.

## Face-supported follow-up result

The new face-supported candidate and every-frame visual findings are recorded in [FACE_SKIN_REVIEW.md](FACE_SKIN_REVIEW.md), with exact evidence in [FACE_SKIN_EVIDENCE.json](FACE_SKIN_EVIDENCE.json). Known architectural false positives and the side-lit face improve, but profile/frame-edge misses and abrupt protection dropouts remain. Continuous normal-speed perceptual playback NOT RUN; no AE promotion. The request for complete flicker acceptance remains open.

Reproduce with external models from `assets/models/face-skin-research-manifest.json`: add `--face-model`, `--detector-model`, `--analysis-scale 2` to the existing trial. Scale2 decodes720×1280 directly from source; balance/native output remains360×640. Run `tests/face_skin_tests.py`, `face_skin_scene_trial.py` for known-error scenes, and `face_skin_motion_review.py` for every-frame evidence. None of the tools automatically grants visual acceptance.
