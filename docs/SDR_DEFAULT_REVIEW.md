# SDR-DEFAULT-REVIEW-001 — current defaults in actual AE

2026-10-18; product commit0fe12f5, installed signed49235442fbda89fd unchanged. User authorized a real-material Original → Auto Prepare → Natural → cinematic comparison, inspection of skin/white fabric/shadows/saturated colors, and corrections only if defects were found. No installer/release/model expansion.

## Inputs and explicit settings

Reused three known shots from the official DPReview Nikon Z6 Flat reel: kitchen20–23s, performers60–62s, clean wide82–84s. The publisher already tweaked this reel; these are edited real SDR, not untouched camera originals. Original source/watermark and earlier source-dissolve rejection remain preserved. See REAL_FLAT_SDR_TRIAL.md and SDR_GRADE_REVIEW.md. A separate72-frame control explicitly reduces contrast in encoded RGB by0.12+0.65*source and quantizes once to8bit. It is labeled artificial everywhere.

All four cases use actual installed AE32bpc unmanaged/sRGB, fresh defaults: Auto PrepareOn, Natural ContrastOn, neutral exposure/contrast/saturation/rolloff, Look Natural, SmartOff. Cinematic variant changes only Teal&Orange65%. No manual tuning is hidden. Each clip has fixed anchors0, floor((N-1)/2), N-1. Auto-only versus default Natural differs only in the neutral Natural Contrast checkbox; the rendered anchor comparisons differ by at most one export code. The four-stage boards show their expected visual agreement rather than inventing another Natural transformation.

## Actual checks

- PASS496actual AE exports:240Natural,240cinematic,12Auto-only anchors,4Original checks; own project saved/reopened with four default-Natural comps verified.
- PASS every decoded output size960×540 and alpha255 on these opaque inputs; source imports within one8bit export code with unchanged alpha. All480processed frames agree with independent current C++ references after quantization within one8bit code. Independent core240+240float results are finite/alpha-preserving and SDR-bounded except existing negligible post-LUT float residue.
- The stricter float-versus-export diagnostic is **not exact**: maximum1.001368codes at anchor samples. Initial exact-source and1code+1e-6float assertions are preserved as failures with causes/observed bounds; the rounded-reference pixel gate is separate. No color algorithm was changed to hide a difference.
- PASS four H.26424fps1920×624deliveries fully decode: real Natural14s, real cinematic14s, artificial control Natural6s, artificial control cinematic6s. Each repeats its shots twice at normal speed. Aspect/watermarks and readable Russian header/footer inspected; four-stage boards and four-time face/fabric crops generated.
- PASS normal owned AE exit, helper archived, installed product inventory/signature unchanged. Existing scenes/projects/caches remain preserved.

## Visual findings and limits

Judgment from inspected full-size frames and four-time crops: Natural is restrained on already edited real scenes; the artificial flat control gains visibly stronger tonal separation and brighter white uniforms. Teal&Orange65% adds warm highlights/skin and cooler shadows while sampled eyes/hair/fabric remain distinguishable. No obvious new hard patches, duplicate outlines or strong color discontinuity in inspected samples. White regions already clipped in the original are not recovered.

Mean per-channel correction changes between adjacent frames stay below0.047codes for real Natural and0.289for real cinematic; the artificial control stays below0.784. This is a global statistic, **not proof that local perceptual flicker is absent**. Fixed per-shot recipes and complete core/native comparisons establish a deterministic color path. User feedback after delivery: «мерцаний не вижу». Accept absence of visible flicker for the shown comparison clips on this material; the exact subset watched was not specified. This does not qualify other sources, SmartOn or universal artistic/Log-equivalence quality. Smart is deliberatelyOff here to evaluate the color base; this does not requalify segmentation or all skin tones. No claim of universal Hollywood quality, untouched-flat-camera coverage or recovered Log/RAW latitude.

No concrete product defect was found in this scoped review, so the installed algorithm/defaults remain unchanged.

## Delivered artifacts

External workspace: `../sdr-default-review-20261018`.

- `natural-real.mp4`, `cinema-real.mp4`: source on the left, actual AE output on the right, three real scenes.
- `natural-flat-control.mp4`, `cinema-flat-control.mp4`: clearly labeled artificial control.
- `{kitchen,performers,wide,controlled-flat}-four-stages.png` and `*-motion-crops.png`.
- `host/Default-SDR-Comparison.aep`: editable Original/Auto-only/Natural/cinematic comps.
- `native-verification.json`, `core-verification.json`, `delivery-verification.json`, `evidence-final.json`, `host/cleanup.json`: complete local records.

Tracked summary: SDR_DEFAULT_REVIEW_EVIDENCE.json. Next step is to review these finished clips artistically and specify any concrete unwanted color change, rather than expanding restoration/AI/GPU or rerunning unchanged LUT checks.
