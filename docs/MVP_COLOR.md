# Color-first MVP — current user direction

Authority: direct user messages on 2026-10-14. First establish useful processing before acceleration; then prioritize color of flat inputs for grading «как с логарифмом». All other enhancement is after MVP. This narrows the delivery sequence, without deleting v3/history or research.

## One material unknown

Pending user clarification: (1) ordinary flat SDR with convenient grading controls, (2) true camera Log to display/output space, or (3) SDR-to-Log output for downstream Log LUTs. These require different math and metadata. No transform/camera profile is inferred from low contrast. Source space must remain explicit, including transforms already performed by AE.

## Acceptance retained regardless of route

| ID | Observable result / check |
| --- | --- |
| MVP-C01 | Independent C++ color core runs without weights, Python, cloud or inference backend |
| MVP-C02 | Typed explicit transfer/primaries, row stride, alpha and memory lifetime; no automatic Rec709 assumption |
| MVP-C03 | Neutral controls and Mix0 give exact packed pixel copy; unchanged alpha and hidden RGB at alpha0 |
| MVP-C04 | Correct unpremultiply/process/re-premultiply; finite float, negative and above-one values preserved by declared policy; no global clamp to make LUT fit |
| MVP-C05 | Grading operations have documented units, ranges, defaults, pivot and operation order; independently calculated fixtures verify them |
| MVP-C06 | Same settings/input give same result regardless of frame order/thread; no framewise auto-color drift or hidden analysis state |
| MVP-C07 | Original versioned looks have explicit spaces; LUT strength and Mix are separate; projects retain exact look version |
| MVP-C08 | Inspect rendered color ramps and representative flat material; exposure/contrast/color assessed separately from mathematical validity; no lost-highlight recovery claim |
| MVP-C09 | Native AE adapter handles actual 8/16/32, rowbytes/ROI/scale/cancel/alpha, save-reopen/Undo and MFR under identified build; offline tests do not establish host runtime |
| MVP-C10 | Build and focused native checks on this Mac; Windows is a separate required portability check, NOT RUN locally |

Controls/automatic behavior, supported input/output profiles and precise artistic acceptance will be finalized from the answer. Existing color-only LUT slice is reusable; existing AI restrictions do not define the color-only input range.

## Independent common grading kernel

Implemented reference operation in include/fstr/color.hpp and src/color/grade.cpp, independently of input/output transform selection. It requires explicitly supplied linear RGB with sRGB/Rec709 primaries. No guessed camera Log, no automatic analysis, no profile conversion or AE acceptance is implied. It is not yet wired into the render/UI path while the interpretation is pending.

Order: uniform exposure gain exp2(stops), then per-channel signed-log contrast, then saturation around linear Rec709 luminance. Exposure[-10,10] default0; contrast[0.1,3] default1; saturation[0,2] default1. Contrast uses fixed pivot0.18 and toe0.01, g(x)=sign(x)*ln(1+abs(x)/toe), inverse sign(z)*toe*(exp(abs(z))-1); f(x)=inverse(g(pivot)+contrast*(g(x)-g(pivot))). Pivot is a chosen grading coordinate, not camera middle gray. Contrast changes black level and can produce negative/above-one values, without clipping. Per-channel contrast can change hue/saturation; this is not a hue-preserving algorithm. Saturation uses Y=0.2126R+0.7152G+0.0722B and Y+s*(RGB-Y), preserves numerical Y but not perceptual hue. Neutral returns exact input samples, including signed zero. Invalid settings/nonfinite results fail explicitly.

Input domain is finite float range; double intermediates are used for precision, not a promise to accept all finite double values. Output can exceed float and must be checked at the future adapter conversion. This bound fixes a reviewed intermediate-overflow counterexample outside the intended float input domain. No clamp is introduced.

These formulas are a bounded mathematical foundation to review, not an accepted finished artistic result. Contrast can strongly move the black point (contrast2 maps0 to-.18); the full research parameter range is not an accepted UI range. Actual flat-source and motion color QA, balance/tonal behavior, profile selection, pipeline integration and native UI remain open.

## Deferred and preserved

AI restoration/defect estimation, denoise/detail/compression, temporal restoration, training, GPU acceleration and Fast/Best model budgets are post-MVP. Six-look v3 library, auto-color and skin-protection obligations are not silently removed; their exact MVP/post-MVP allocation follows the clarified color workflow. Custom LUT import remains post-MVP until explicitly changed. No installation, merge or release has occurred.

The completed quality-v1 run is an archived research result, not the current development objective. A 27.6ms near-identity student failed quality; it is not a product candidate.
