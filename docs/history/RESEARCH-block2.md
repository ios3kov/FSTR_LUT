# Preserved research block 2 — 2026-10-14

This is historical evidence. User subsequently moved all restoration/training/acceleration after a color-first MVP (MVP_COLOR.md). No research model was accepted for production. Nothing was installed or released.

## Corpus and domains

Tears of Steel official Xiph-hosted movie, SHA256328253280a05e6b14a92f1f43d818d9b7d38ab5d3f6d5d98606198fd3daef2ff,571346576bytes. Film CC BY3.0 per https://mango.blender.org/about/. Full credits remain in local source; technical QA scope, no actor endorsement/advertising. Container has no color tags: Rec709 limited SDR interpretation is explicit research assumption, converted to encoded sRGB. Film is already graded/VP8-compressed, not clean camera ground truth.

96 frames at768x320/12fps. Entire scene groups split before patches: day_outdoors train; lab_interiors validation; future_exteriors test. Corpus manifest and source hashes are external at ../research-block2/corpus/frames/manifest.json.

SIDD official homepage illustration at https://abdokamel.github.io/sidd/:1700x600 noisy/reference PNG. Authors explicitly state MIT for dataset and code. One published example, not held-out certification; NAFNet trained on SIDD. Source images/hashes retained externally. Data rights do not establish model-weight redistribution rights.

## Candidate measurements before priority change

FFDNet gray/luma485316 parameters: warmed1080p MPS median208.95ms. Equal encoded RGB delta preserves encoded chroma, therefore color noise remains; luma noise sigma projected from RGB variance. IRCNN color188163 parameters: median496.52ms. Both fail the Fast model-path target on this Mac. Smaller parameter count did not imply faster execution.

Residual student21,060 parameters, fiveConv layers, identity initialization,1200bounded training steps, explicit source/teacher recipe: warmed model path27.60ms1080p. **Quality FAIL**: validation_lab input26.76746dB vs student26.76740; validation_people26.60488 vs26.60480. It is essentially a near-identity correction. Excluded, not integrated with C++ backend. No further training after user's quality/color priority change. CPU/MPS move was separately checked and did not change parameter objects/optimizer membership; no proven root cause claim. Teacher-derived weight rights remain unresolved.

SplitterNet inspected code licence CC BY-NC-SA4 academic scope: excluded commercial candidate; no weights/inference downloaded. Existing teacher/model source pins and original negative results retained.

## Frozen quality-v1 comparison

Completed 21 cases: four1-second moving clips (night/face/action/rooftop), source, addedGaussian5/15, extraH264CRF34, modestGaussianBlur0.6; plus real-noise SIDD illustration. Same input per method, seeds frozen. Compare Input, ordinaryblur0.6, FFDNet matched-or-weak5/15, fixed15 stress, IRCNN corresponding banks2/7, NAFNet-SIDD32. Unclamped float PSNR/gradient/detail-ROI/RGBbias/range/temporal-difference errors recorded per frame and scene. Source identity PSNR is capped120dB; source-as-is metrics evaluate preservation, not objective improvement. Do not combine those scores with degraded-input improvement.

Representative findings: face Gaussian5 FFD matched+10.857dB and IRCNN matched+10.678dB vs input; Gaussian15+15.289/+14.774. Fixed15 on Gaussian5 visibly smooths fine facial structure. ExtraH264 gains small and scene-dependent; deblur was not established (these are denoising weights). SIDD illustration NAFNet+14.972dB vs input, FFDfixed15+2.327, IRCNNfixed15+2.465, ordinaryblur+4.812. Model usefulness depends strongly on training/input domain. No general-purpose Enhance acceptance.

Root inspected the face Gaussian5 still in original-size320x256 detail crops: fixed15 substantially softens wrinkles/eyelids, matched weak variants preserve more. Continuous video playback review NOT RUN; no flicker/face-preservation certification. quality_preview.py exists but was not executed after the color-first reprioritization.

External artifacts: ../research-block2/quality-v1/quality.json, quality-v1.log, compact-mps/results.json, student-1200/recipe.json, student-export/validation.json. No footage or weights committed. Optional65536-sample transfer tables remain off by default; their dense numeric and extended-fallback checks passed, but speed/MVP visual adoption is not claimed.

Senior read-only consultation helped define matched sigma, independent input baseline, no-op/detail vetoes and motion-relative error; reviewer did not run tests/edit source. Root separately reviewed actual source and performed checks.
