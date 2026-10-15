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

Research inference/analysis runs in a separate Python environment outside the delivered plugin. Reusable pointwise look/protection math is C++17 without AE headers. Prototype input is explicitly prepared straight opaque linear Rec709; analysis sees an sRGB display copy. Learned skin confidence is not a calibrated matte or a hue-only skin detector. Model load occurs once per research run; models/footage stay outside Git and plugin packaging.

Order: explicit input decode -> conservative Auto Balance -> original frozen LUT -> selective chroma protection. This prototype does not add new manual grading controls or alter existing plugin defaults. Natural stays exact when Smart Color is off. For future integration, manual grade and shoulder belong after balance, before LUT; global Mix remains last.

Let B be prepared linear RGB and L the LUT result at selected strength. Protection removes the color-difference part of L-B: O=L-p*((L-B)-Y(L-B)*(1,1,1)). This preserves linear Rec709 Y, not perceptual saturation, white balance or chromaticity. Darkened/extreme looks can create negative channels; preserve finite floats in the kernel, count display clipping in research outputs, do not hide it with core clamps. Low-confidence mask fades protection to zero.

Auto Balance is a bounded aesthetic heuristic, not scene-intention recovery. It must not force every median to camera middle gray. White balance requires distributed low-chroma candidates with consistent ratios; limited evidence yields identity. Night/colored-light artistic acceptance is separate from bounded output.

Temporal neighbors are explicit. Flow is computed from pre-look display copies; analysis is independent of selected LUT/strength. Cut detection is a heuristic with known flash/gradual-transition and similar-palette hard-cut ambiguity; synthetic boundary tests supplement real motion. Only supplied/detected boundaries are guaranteed not to be crossed. Identity/uncertain central exposure and WB remain identity even if neighbors suggest a correction. Any integration must expose/cache these dependencies to AE rather than inheriting sequential VIDEO-mode state.

The retained candidate uses full-frame inference plus four fixed overlapping semantic views, with feathered confidence fusion; masks retain current-frame dominance. This addresses missed small subjects without moving crop geometry. It does not solve semantic false positives. Earlier full-frame/coarse-person-crop trials are retained as negative evidence outside Git. Body/face weights use smoothstep0.45..0.85 and strength80% in comparisons; thresholds are research choices, not calibrated probabilities.

Observed independent-corpus failure: the model selected architectural balusters in two complex-light scenes and missed much of a visible face in one; a uniform beige negative control also produced a small false positive. SC02 overall quality is FAIL. This candidate must not be promoted into the installed AE plugin. No denoising/other enhancement has been added to compensate for this.

## Reproduction

Use a separate Python3.12 arm64 environment with `tools/benchmark/smart-requirements-lock-macos-arm64.txt`. Model location is external; its official pinned URL, SHA, label order and actual metadata normalization are in `assets/models/smart-skin-research-manifest.json`. Metadata extraction's high-level Python helper was unavailable in the installed wheel; direct FlatBuffer metadata inspection verified input/output shapes and mean/std127.5. Runtime IMAGE inference itself succeeded.

Build `fstr_smart_render` and run core CTest normally. Run `tests/smart_analysis_tests.py` in the research environment. `tools/benchmark/smart_color_trial.py --help` describes the private opaque SDR portrait/Teal trial. Every trial requires new job/output paths and an explicit input transfer; it snapshots executed Python files. Source preview is360x640/25fps, capped156frames; it is not a full-resolution or AE performance test. Native stream files are same-machine little-endian research data, not a public interchange format. Fresh output is refused if a target already exists; a failed stream may leave an incomplete research file and reports failure.

`smart_scene_trial.py` reuses the hash-checked existing seven-scene Tears of Steel research corpus and three authored negative controls. Film attribution is displayed on its output. Existing split names are retained historical names; this experiment does not train or tune against those scenes. Portrait/still evidence does not establish all skin tones or full video acceptance.

## Sources

- [Official MediaPipe Image Segmenter](https://developers.google.com/edge/mediapipe/solutions/vision/image_segmenter): zero-based 2 body-skin, 3 face-skin, IMAGE mode/confidence output.
- [Official multiclass model card](https://storage.googleapis.com/mediapipe-assets/Model%20Card%20Multiclass%20Segmentation.pdf): Apache2.0 model, no pixel-perfect matte guarantee; low light/backlight/motion/occlusion limitations. Download is pinned by SHA256 in the model manifest; weights are not redistributed in this milestone.

Current progress and exact evidence will be recorded in STATUS.md and SMART_COLOR_EVIDENCE.json. Original MVP/Windows/Log/HDR/AE obligations in PLAN.md remain open.
