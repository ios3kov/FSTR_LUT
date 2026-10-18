## Full Core ML Smart validation — 2026-10-18

**SMART-COREML-VALIDATION-001: numerical/native trial PASS; production loading NOT QUALIFIED.** Frozen experimental candidate8ef20895ffc59d55 passes168full observations,12out-of-order repeats,174same-shot window checks,432look/strength and168sequence color comparisons. Actual AE25.6 completes28exports/18pixel comparisons within1PSD exportcode, including8/16/32paths, cuts, toggle and LUT reuse. Three initialized cold-request pairs each run5real inferences: medianCPU16035ms versusCoreML8645ms (46.09%less elapsed,960x540). Warm pairs use masks and do not measure model acceleration. CoreML effect creation still takes57827ms despite standalone prewarm; no default promotion. Own AE exited; trial/helper archived; working installed1c490d4515b2937b inventory/signature unchanged. Next: managed background initialization/lifetime, fixed backend per request and actual AE cancellation/loading validation. [Contract, exact evidence and limits](SMART_COREML_VALIDATION.md). No installer/release.

# Current preparation acceleration — 2026-10-18

Installed Current1c490d4515b2937b adds bounded prepared-input reuse to the prior cache/color improvements. Accepted model/color/defaults and raw disk keys are unchanged; actual upstream/format invalidation and15full image/hash comparisons pass. Measured warm-frame median1251→751ms is scoped to3paired960x540/32bpc requests, not real-time FPS.22Release/22sanitizer tests;51successful host exports across identified jobs; prior ed9/accepted42ee preserved. CoreML is a separate measured prototype, not the installed model backend. [Exact contract/results](SMART_INPUT_PREPARATION.md). Earlier milestones below retain their historical artifact identities.

# Current combined acceleration — 2026-10-18

Installed Currented9e33d687019dc1 includes the cache improvement and exact common-LUT reuse. No profile, controls, mask/model or accepted-look change. Frozen276480records match exactly; actual paired warm960x540render median1764→1238ms, with canonical host verification. Previous7be and accepted42ee preserved. [SMART_COLOR_REUSE.md](SMART_COLOR_REUSE.md) records output/timing scope; faster model inference, source hashing and production playback FPS are not established.

# Current cache improvement — 2026-10-18

Working installed Current is7be976a9a5160b0e after SMART-CACHE-LATENCY-001: unchanged color/model math and controls; accepted42ee remains archived byte-exact as quality reference. Actual18baseline comparisons across24native exports pass <=1PSD8code; masks persist across launches and complete5-frame windows stay in RAM for LUT/strength edits. This does not establish fast playback or accelerate the color kernel. [Details](SMART_CACHE_LATENCY.md).

# Accepted quality and acceleration baseline — 2026-10-18

User accepts shown quality of42ee1edaaff01753. Preserve that bundle and the linked accepted comparison artifacts while accelerating. SMART-STAGE-PROFILE-001 measures six standalone inputs and16AE diagnostic exports; it changes no color, mask, model, encoding defaults or installed product bytes. Actual RAM-warm Smart still takes1509ms at960x540 in the scoped trial; no real-time claim. [Measurements and next work](SMART_STAGE_PROFILING.md).

# Scene-aware SDR preparation — 2026-10-18

Auto Prepare SDR now supports a complete bounded clip scan with independent correction per detected shot. Advanced → Detect Scene Changes isOn for fresh effects andOff for missing old-project values. Off retains the established three-anchor behavior. Explicit Log/3D paths and encoding defaults are unchanged. See [SDR_SHOT_DETECTION.md](SDR_SHOT_DETECTION.md) for187native exports, cache/compatibility evidence,16384-frame limit and unverified cases.

# Explicit input extension — 2026-10-18

INPUT-PROFILES-001 now appends ARRI LogC3 EI800/AWG3, LogC4/AWG4, Panasonic V-Log/V-Gamut, ACEScct/AP1 and ACEScg/AP1 to the original Host/Sony choices. This extends the initial Sony-only scope below under the user's later authorization. Output remains the explicit existing host Rec709 transfer; source normalization is not an ACES display rendering transform. See INPUT_PROFILES.md for exact checks and remaining formats.

# Current encoding default decision — 2026-10-18

User explicitly selected sRGB by default with manual override and canceled Auto altogether. This supersedes older unspecified-default/automatic-discovery obligations below. Existing saved encoding values retain their meanings; source Log versus already-converted Host remains explicit. See docs/COLOR_AUTO_FOLLOWUP.md SRGB-DEFAULT-001 for compatibility acceptance. Historical requirements/results below remain tied to their original artifacts.

# Color-first MVP — current user direction

Authority: direct user messages on 2026-10-14. First establish useful processing before acceleration; then prioritize color of flat inputs for grading «как с логарифмом». All other enhancement is after MVP. This narrows the delivery sequence, without deleting v3/history or research.

## Accepted input workflows

User selected both ordinary SDR grading and true camera Log processing. SDR-to-Log export is not the current MVP. User delegated the initial camera/profile choice; select Sony S-Log3 / S-Gamut3.Cine as a practical starter, not a quantitatively proven market-share winner. ARRI documentation is preserved research; additional Log profiles are later extensions, no generic Log selector. Source space remains explicit, including transforms already performed by AE. Never infer a camera curve/gamut from flat appearance.

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

Host samples/output: explicit sRGB, linear Rec709 or Gamma2.4 Rec709, full-range float. Source override: Host (AE already prepared material) or preserved/unconverted Sony S-Log3/S-Gamut3.Cine samples. If AE already applied Log input conversion, use Host to prevent double transformation. SDK host-space discovery remains a separate required integration check. No automatic color/noise analysis in this deterministic first slice.

Warm Film v1 uses tetrahedral interpolation in encoded sRGB, with clamp-to-domain LUT sampling. Only the sampled look branch clamps its coordinates; the base grading branch is not globally clipped. LookAmount0 bypasses LUT sampling. This is not an extended-gamut look.

Global Mix is a linear host/display appearance crossfade: original raw samples interpreted as host output encoding versus the complete processed result rendered into host-linear coordinates. Mix0 is an exact raw copy; Mix1 computes only processed branch. Intermediate Mix attenuates Log normalization and grading together. It is not camera-radiance interpolation. Neutral Host copies exactly; neutral Sony still performs the selected input normalization.

Technical Log normalization decodes the official Sony full-range curve and applies the gamut matrix to linear Rec709. This alone is not a finished Sony viewing LUT. Optional explicit luminance shoulder is a creative SDR rendering stage before Look/Mix: knee0.6, identity below knee, S(Y)=k+(1-k)*(Y-k)/(1-k+Y-k) above; scale RGB uniformly by S(Y)/Y and blend with the original by shoulder amount. Default0 preserves extended values; no hidden clamp. Saturated out-of-gamut channels can remain negative/above-one: this is not gamut mapping or recovered missing detail.

## Independent common grading kernel

Implemented grading in include/fstr/color.hpp and src/color/grade.cpp. It requires explicitly supplied linear RGB with sRGB/Rec709 primaries. It is now integrated through one immutable ColorProcessor, used by standalone render_color and the native adapter. profiles.cpp implements official Sony S-Log3 inverse/forward and the derived S-Gamut3.Cine-to-Rec709 matrix. Gamma2.4 negative handling is an explicitly signed extension, not a BT.709 OETF. No automatic profile inference or color analysis.

Order: uniform exposure gain exp2(stops), then Temperature/Tint creative gains, then per-channel signed-log contrast, then saturation around linear Rec709 luminance. Exposure[-10,10] default0; contrast[0.1,3] default1; saturation[0,2] default1. Contrast uses fixed pivot0.18 and toe0.01, g(x)=sign(x)*ln(1+abs(x)/toe), inverse sign(z)*toe*(exp(abs(z))-1); f(x)=inverse(g(pivot)+contrast*(g(x)-g(pivot))). Pivot is a chosen grading coordinate, not camera middle gray. Contrast changes black level and can produce negative/above-one values, without clipping. Per-channel contrast can change hue/saturation; this is not a hue-preserving algorithm. Saturation uses Y=0.2126R+0.7152G+0.0722B and Y+s*(RGB-Y), preserves numerical Y but not perceptual hue. Neutral returns exact input samples, including signed zero. Invalid settings/nonfinite results fail explicitly.

Input domain is finite float range; double intermediates are used for precision, not a promise to accept all finite double values. Output can exceed float and is checked at the implemented pixel/adapter conversion. This bound fixes a reviewed intermediate-overflow counterexample outside the intended float input domain. No clamp is introduced.

These formulas are a bounded mathematical foundation, not an accepted finished artistic result. Contrast can strongly move the black point (contrast2 maps0 to-.18); the full research range is restricted in native UI. Actual flat-source/camera/motion color QA, white-balance/tonal refinement and full native host acceptance remain open.

## Current native development slice

SDK25.6, macOS arm64 development bundle with explicit source profile, exposure, Temperature/Tint, contrast, saturation, highlight rolloff, Neutral/WarmFilm(v1), LookAmount, Mix and AE Pixel Encoding. Native contrast UI deliberately restricts the unaccepted wide research range to0.5..1.5. Unknown pixel encoding defaults to safe neutral bypass, active processing asks for explicit selection. When the AE project linearizes sRGB/Rec709, choose Linear. SDK profile discovery, automated color correction, final six-look library, presets and cross-platform packaging remain tracked work; this is not a finished product.

Native RGBA uses actual SDK ARGB fields and PF_MAX_CHAN16=32768; buffer premultiplication contract is premultiplied stored samples. Neutral/Mix0/alpha0 copy native structs before conversion. 8/16 output quantizes/clamps to the declared integer channel range; 32 output preserves finite extended values within float. SmartFX origin and rowbytes mapping is pointwise; no downsample multiplication or spatial halo. Empty output succeeds, empty input returns transparent without asking its null pixel format. Parameter/pixel/suite releases are explicit on success and error; immutable LUT snapshot is loaded from embedded bytes at GlobalSetup.

Build identity hashes source, relevant SDK headers and toolchain/config and follows changes without manual configure. PiPL is packaged in the data fork using Rez-useDF; source-only resource incremental test changes ID/rebuilds, then restores exact source/ID. A local ad-hoc signature is development provenance, not notarization/release certification.

Functional evidence: core/color suites; owned SDK callbacks on the actual dlopened bundle; full-frame authored-flat SDR and synthetic Sony-Log fixtures max4.31e-7 vs reference, ninecases. Root visually inspected face comparison at1536x1044. Synthetic encoding is not an actual camera clip. AE loader/host/real MFR and real-camera color acceptance must be reported separately.

## Deferred and preserved

AI restoration/defect estimation, denoise/detail/compression, temporal restoration, training, GPU acceleration and Fast/Best model budgets are post-MVP. Six-look v3 library, auto-color and skin-protection obligations are not silently removed; their exact MVP/post-MVP allocation follows the clarified color workflow. Custom LUT import remains post-MVP until explicitly changed. A uniquely owned Mac development bundle is installed for validation; no existing plugin was overwritten. No merge or release has occurred.

The completed quality-v1 run is an archived research result, not the current development objective. A 27.6ms near-identity student failed quality; it is not a product candidate.

Temperature/Tint are creative gains, not Kelvin/CCT recovery: R=2^(temperature/2), G=2^(-tint/2), B=2^(-temperature/2), divided by their Rec709-luminance-weighted sum. Core range[-1,1], native UI[-100,100], default0. Uniform exposure precedes this balance; contrast/saturation follow. Neutral white luminance is preserved, positive temperature warms and positive tint moves toward magenta. No framewise auto-balance or camera white-point inference. Persistent IDs1..9 retain meaning; new Temperature/Tint use10/11 despite reordered presentation.

## Latest actual-host checkpoint

Source8733709 / BuildID32597669ec8fb8e1 is a limited Mac trial. Actual metadata and camera SDR still inspected; current32-bpc saved-project SDR/SyntheticSony/WB/alpha regression passes10 exports. Historical356 actual8/16/32/state/linear-space results remain source-bound evidence. Current full-camera float oracle, actual8/16/extended float/ROI/scale/gradingUndo/preview, realLog/motion/Windows and incoming-space discovery remain open. STATUS.md is the canonical current result; old paragraphs above describe their original implementation checkpoints.


## Cinema & Palettes delta — 2026-10-15

Direct user steering supersedes the old six-name library: Natural exact neutral plus15original film/palette looks. Warm v1 remains only for legacy value2 behind a nonselectable separator. Strength defaults100%, soft0..100%, typed0..300%, extrapolated in the existing host-linear mixing space. Full contract and original palette source scope in CINEMA_LOOKS.md. The current implementation is fixed pointwise3D LUT processing, without scene/skin awareness or adaptive relighting. Source158682d / BuildIDb7f44ed4ca0704d6 current scoped checks are in STATUS.md/EVIDENCE_CINEMA_V1.json; earlier checkpoints above retain their original scope.
