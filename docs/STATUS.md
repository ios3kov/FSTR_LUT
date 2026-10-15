# FSTR-LUT — current development checkpoint, 2026-10-15

## Skin-colored occlusion acceptance — current research checkpoint

[Scope and negative evidence](SKIN_OCCLUSION_ACCEPTANCE.md), [exact report](SKIN_OCCLUSION_ACCEPTANCE_EVIDENCE.json). Added structural correspondence guard against false temporal recovery.84 paired synthetic cases improve skin-colored texture rejection, with measured loss of valid surrounding support.156-frame native masks/render and23 static scene results unchanged from af88cb0; installed plugin unchanged.

**FAIL semantic occlusion:** actual pinned FaceOcc accepts planted skin-colored patches in real footage before recovery (interior mean0.752/0.826). This cannot be fixed by relaxing recovery thresholds. No AE promotion. **Continuous visual review pending:** user could not confidently assess the small-face overview; enlarged three-loop preview created and started in QuickTime. Tool snapshots are not continuous perception. Earlier missing-mask intervals persist. Do not mark either requested gate closed without evidence.

---

## Blur recovery — current research checkpoint

Implemented two-sided motion-checked recovery for a relative-blur dropout, with inward directional feather. [Scope/results](BLUR_RECOVERY.md), [exact evidence](BLUR_RECOVERY_EVIDENCE.json).

**PASS in inspected scope:** frame55 protection partially restored (mask mass1.34→99.25, prior support181.62). Other155 frames have bit-identical masks and native renders.23 static scene results unchanged, including lab prop rejection. Synthetic translation, cuts, weak seeds, sharp input and distinct-color transient occluder tests pass; actual nonchronological repeats and MP4 full decode pass. Installed plugin unchanged.

**Not fully accepted:** masks remain partial and inherited profile/blur misses persist. Similar-color moving occluders and multiple-face motion unverified. Continuous normal-speed flicker review and whole-sequence native-scale review **NOT RUN**. Visually inspected changed frame55, neighboring54/56 at full scale and48–59 detail. No AE integration/install/release. No additional threshold tuning to this one clip without independent cases. Earlier obligations below remain preserved.

---

## Visible-face occlusion — current research checkpoint

Implemented independent pinned FaceOcc gate with fixed roll hypotheses and strict input contracts. [Review and open criteria](OCCLUSION_REVIEW.md), [exact evidence](OCCLUSION_EVIDENCE.json).

**PASS in inspected scope:** lab prop false coverage removed while forehead/cheek diagnostic regions retain100%/97.76% of previous support; no new background coverage. Actual156 frames and23 scene cases processed; four Python suites and static scanner pass, five installed bundle hashes unchanged. Comparison MP4 fully decodes.

**FAIL / not accepted:** blurred frame55 loses99.26% of previous support; some tilted faces lose partial support and inherited profile/edge misses remain. All156 face crops,23 scene cases and five full-frame panels inspected. Continuous normal-speed flicker review and all-frame native-scale review remain **NOT RUN**. No AE integration, installation, Fast claim, weights redistribution or release. Next investigation is motion-compensated neighbor agreement with explicit occluder rejection, not unconditional mask blur. Prior requirements and evidence below remain preserved.

---

## Turn recovery — current research checkpoint

Implementation `695235a7d8e91491e68e5e0967d6313dfd1623f1`. Added profile fallback and nonrecursive short-gap transport in the independent prototype; installed AE is unchanged. Scope and results: [TURN_RECOVERY.md](TURN_RECOVERY.md), exact evidence: [TURN_RECOVERY_EVIDENCE.json](TURN_RECOVERY_EVIDENCE.json).

**PASS in inspected scope:** six small-face detector misses restored; frame55 recovered from neighbors54/56 (also independently selected by profile fallback). Seven previously absent masks restored,23→16 absent frames including actual face departure. Model/default repeatability exact, radius2 contract tested, zero-mask background exact. Three Python suites, five Release and five ASan/UBSan native suites pass; both156-frame comparison MP4s decode fully. Installed b7f44 five hashes unchanged.

**FAIL / not accepted:** visible faces still missed on40/41,131/132,140/141,146; inherited lab prop occlusion leak persists. Profile/edge coverage incomplete. All156 frames inspected in overview/detail, ten selected native-scale full-frame sheets and23 scene cases reviewed. Continuous normal-speed playback and full native-scale review of all frames **NOT RUN**; user's flicker acceptance remains open. Output: `outputs/FSTR-LUT-Turn-Recovery-v1` and `outputs/FSTR-LUT-Turn-Scenes-v1`. No AE integration, Fast claim or release. Earlier evidence/obligations below remain historical and preserved.

---

## Face-supported skin follow-up — current research checkpoint

Implementation `180304bfaf56ad2a671f7c8dc7b924bb45352614`. Independent YuNet proposals, IMAGE-mode face confirmation and semantic skin intersection implemented; face-only scope, no hands/body. Actual720×1280 source analysis feeds the same360×640 native color comparison. User's requested known-error and motion follow-up is recorded in [FACE_SKIN_REVIEW.md](FACE_SKIN_REVIEW.md), exact sources/artifacts in [FACE_SKIN_EVIDENCE.json](FACE_SKIN_EVIDENCE.json).

**PASS in inspected scope:** prior baluster/beige false positives removed and side-lit face selected in the ten-case known-error comparison; all156 frames reviewed in overview and enlarged detail. Numeric checks preserve background/current support, baseline decode/LUT/balance and nonchronological repeatability. Five Release and five ASan/UBSan core suites, both native CLI checks, face/analysis tests pass. Actual high-resolution decoder completion checked; injected nonzero decoder exit is rejected before native render/evidence. Both final comparison MP4s decode156 frames at25fps/6.24s.

**FAIL / not accepted:** visible profile and frame-edge faces remain missed; protection drops abruptly at37–44,55,131–132,140–141 and146 (later frames include face departure). Temporal smoothing does not fix these detection failures.23 zero-mask frames is not an accuracy score. Continuous normal-speed perceptual playback and complete whole-frame native-scale review are **NOT RUN**; the user's full flicker-acceptance requirement is still open. Exact coverage is seven overview sheets, twenty detail sheets and four selected native-panel sheets, not a claim that every generated artifact was inspected.

Final local outputs: `outputs/FSTR-LUT-Face-Skin-v3` and `outputs/FSTR-LUT-Face-Skin-Scenes-v2` outside Git. Research timings on this Mac: face/skin analysis median266ms,p95352ms at720×1280; temporal199ms and native smart36.9ms at360×640. These are stage diagnostics, not1080p/AE/Fast results. Source/LUT/protection arithmetic leaves finite extended values; observed display out-of-range fraction0.511% includes unchanged preparation/look effects, not solely face protection. Gamut/artistic policy remains unaccepted.

Current installed AE build remains **b7f44ed4ca0704d6**, five file hashes unchanged. No AE actions/install, model redistribution, release, merge or publication. Next work is reliable uncertain-face transitions and unseen-scene/occlusion validation, followed by actual continuous playback and remaining host gates. Earlier MVP/Log/HDR/Windows obligations and negative evidence below remain preserved.

---


## Smart Color research checkpoint

Implementation `95733b02309da422a926028f901641ad6bc798cd`. User-authorized independent prototype implemented: semantic skin masks from pinned MediaPipe256, full-frame plus four fixed overlapping views; conservative exposure/WB; explicit radius2 flow-gated masks and log-gain/stop smoothing; separate native C++ selective chroma protection. Scope/criteria: [SMART_COLOR.md](SMART_COLOR.md). Exact sources/artifacts and gate results: [SMART_COLOR_EVIDENCE.json](SMART_COLOR_EVIDENCE.json).

PASS: five core Release suites and five ASan/UBSan suites; research analysis tests including translated texture/mask direction, photo rejection, uncertain-center identity, invalid masks, supplied cut barriers and reverse order; actual CLI malformed-stream/bypass/preservation checks in both builds. 156 supplied-video frames at360x640/25fps rendered through native core in five variants. Model repeat requests on frames155/0/78 were exact. Zero-mask pixels exactly equal balance+LUT; linear luminance max difference7.68e-8 after float storage. Source snapshots/current renderer and unchanged installed b7f44 five-file hashes verified.

**FAIL semantic quality / do not promote:** independent seven-scene movie corpus + three authored controls exposed architectural balusters selected as skin and a visible face missed under complex side lighting. Uniform beige field has a small false-positive mask. Selected C2098 stills improve local skin color compared with unprotected LUT as an editorial judgment, but masks remain incomplete; confidence scores are not mask-accuracy evidence. The strong skin blend can produce out-of-range display RGB (~0.584% of pixels on this trial); the native float kernel intentionally preserves those values. A production gamut/skin policy is not accepted.

Two6.24s comparison MP4s and inspected1/2/4s boards/portrait/independent-scenes sheet are local under `outputs/FSTR-LUT-Smart-Color-v3` and `outputs/FSTR-LUT-Smart-Scenes-v1` outside Git. Numerical video processing and selected still inspection are complete; continuous subjective playback/flicker acceptance NOT RUN. Global adjacent-mask differences include real motion and are not a quality verdict. Known cut heuristic misses some similar-color cuts. Complete occlusion ground-truth validation NOT RUN.

Research CPU medians on this Mac: segmentation five views~699ms, temporal~196ms, native smart transform~38ms per360x640 frame. These are diagnostic stage timings under this run, not1080p/AE/Fast measurements. No acceleration claim.

Current AE installation remains **b7f44ed4ca0704d6**, unchanged; no Smart Color UI/backend has been installed. AE/alpha/ROI/MFR/native model packaging and broad skin-tone/realLog/HDR/Windows acceptance remain NOT RUN. Decision: retain this experiment and its negative evidence; next bounded step is a different semantic backend or face-supported mask candidate evaluated on the same negative cases before any AE integration. Existing color MVP obligations below remain open; no release, main change or publication.

---

## Installed Cinema & Palettes checkpoint

Implementation158682d69567445980282b2d7dae70cb292c1bfd; signed Mac BuildID **b7f44ed4ca0704d6**. Natural plus15 original Cinema & Palettes looks with clear English names,0–100% slider/default100%, typed strength up to300%. Old Warm removed from new selection; stable value2 is a separator retaining frozen old-project appearance. No changed diskIDs/order. Current scope/acceptance: [CINEMA_LOOKS.md](CINEMA_LOOKS.md); exact artifact/Evidence: [EVIDENCE_CINEMA_V1.json](EVIDENCE_CINEMA_V1.json).

PASS:6signed Release suites and6ASan/UBSan suites; 23actual AE25.6x101 PSD8 exports,16new-default100% styles,0/200/300%, old Warm and3reopen comparisons. Natural vs saved Original is exact in the inspected export; repeat/legacy/zero comparisons max1/255 at fixed2/255 tolerance. Actual loaded module path and5installed hashes verified. Actual controls show Teal & Orange100%, FX3/SDR and whole frame25%. Old project unchanged; previous plugin and completed one-shot helper fully archived outside MediaCore. Local ZIP8,224,367bytes roundtrip/strict signature PASS.

Artistic judgment is separate: all15styles visibly differ on inspected supplied SDR frames; deliberate strong global color may tint skin. These are authored interpretations, not official studio LUTs, measured popularity ranking, skin recognition or exact-film matches. Natural adds no style. No scene intelligence/auto-adaptation has been implemented.

NOT RUN: manual mouse dropdown opening and slider drag (CUA windowNotFoundAtPosition); user artistic approval; real cameraLog, valid unquantized full-frame/HDR oracle, Windows, broader actual8/16/ROI/downsample/gradeUndo/preview/concurrency/video checks. Those existing MVP obligations remain. AE showed a transient internal verification failure {no current context} before final UI review; an unsaved1px layer move was observed, then own-project Revert restored the exact saved setup and correct position. Cause unassigned; no forced termination or preference change, no save of unexpected state.

**Limited development trial; full color MVP incomplete.** The metadata checkpoint below is retained historical Evidence, not blanket proof of the new candidate.

---

# Preserved metadata checkpoint — 2026-10-14

Color-first scope: ordinary SDR grading and explicit Sony S-Log3/S-Gamut3.Cine normalization. Automatic read-only Sony camera metadata recognition is added. Restoration, denoise/detail/training/acceleration remain after MVP. Branch dev/ai-enhance-prototype; rules v11.1.0 /04b606845e0f73ab28b9807bb45682eae4d6ce34.

**Implementation committed: 8733709c7076da76a95e0510a9b55fd0cb2b7b8a. BuildID32597669ec8fb8e1. Limited Mac trial, full MVP incomplete.** Core color math remains25819a9e379a49caaa94df4fa5e2af5112b5cd07. Documentation checkpoints do not rebuild the bundle. Exact source/file hashes, procedures/results and retained boundaries: [EVIDENCE_COLOR_MVP.json](EVIDENCE_COLOR_MVP.json). Older7fbe evidence is archived unchanged under docs/history;356 host evidence remains a historical reference. No main merge or release.

| Acceptance / inspected scope | Status |
| --- | --- |
| MVP-C01..07 core contracts, parameter units, alpha/extended policy, deterministic color and frozen look; actual dlopened native callback fixture | PASS, five Release suites; corresponding ASan/UBSan suites PASS. Actual AE and external SDK are not instrumented |
| META01..03 structured bounded parser, namespaces/conflicts, whitelist privacy | PASS. Local supplied camera declared Sony ILME-FX3/Rec709,2100bytes read; vendor/license hashes unchanged |
| META04..06 actual camera caption, source replacement fallback, Undo return, unchanged profile/encoding | PASS in nonce-owned AE25.6x101/default Effect Controls panel; no render IO or parameter auto-changes |
| META07 actual manual-refresh click, narrower panel, proxy/missing/composition fallbacks | NOT RUN in actual host. Stand-in callback/error/cache tests PASS; CUA coordinate routing failed for the actual refresh click |
| MVP-C08 actual SDR result | One Original/Grade/Warm portrait comparison inspected. Orientation/framing and subtle tonal/look changes observed; user artistic acceptance, multiple frames/motion and actual camera Log remain open |
| Actual full-frame camera arithmetic | NOT RUN with a valid float oracle. Initial neutral-PSD8 reference failed at clipped whites(max0.04277/0.04752); tolerance2/255 retained. API/CLI OpenEXR-format attempts rejected the read-only property. Negative evidence preserved; no product color defect inferred from that uncalibrated oracle |
| MVP-C09 current candidate old project reopen, SDR/SyntheticSony, WB and alpha, MFR ON50 | PASS10 exports on32-bpc projects vs references, tolerance2/255; max0.006246, alpha export error<=1LSB. Eight PNG8 SDR/Sony, one PNG RGB alpha-on-black, one RGBA PSD8. Actual concurrent callback peak is not measured |
| New candidate actual8/16 projects, negative/HDR float worlds, ROI/downsample, grading-parameter Undo/RAM Preview | NOT RUN. Current owned callback fixture covers8/16/32 and ROI/cancel; historical356 actual8/16/32/state/linear-working-space evidence retained separately |
| MVP-C10 Windows / real Sony Log / other camera metadata / automatic incoming-pixel-space discovery | NOT RUN. Explicit manual incoming encoding remains required |
| Final package/owned installation/cleanup | PASS signed arm64 eFKT/FXTC bundle,5matching files; libc++/libSystem only, static tinyxml2. Old owned bundles archived, exactly one active current FSTR bundle. Saved own AE normally closed; temporary helper archived outside MediaCore |

Separate root final review covered requirements, parser bounds/namespace/UTF16, source handle ownership, Drawbot cleanup, cache/render separation and old diskIDs1..11. The source was independently reviewed earlier by the authorized senior agent; that agent did not execute host/tests. Upstream tinyxml2 CRLF bytes/license remain exact; owned diff whitespace check excludes unchanged upstream files. Whole-product release/security/performance readiness is not claimed.

Next bounded work: establish an unclipped actual-camera reference export, check real S-Log3 footage and short-video color, then remaining actual-host8/16/float/ROI/Undo/preview checks. Inspect actual refresh/narrow/proxy/missing behavior when coordinate routing is available. Windows remains a separate portability milestone. Broader v3 obligations are retained in PLAN.md/MVP_COLOR.md; this checkpoint does not silently discard them.

## Historical checkpoints — retained, superseded

# FSTR-LUT checkpoint — 2026-10-14, color native slice

Current user scope: ordinary SDR grading and true camera Log, first Sony S-Log3/S-Gamut3.Cine. User delegated profile choice. Restoration, model/training and acceleration remain post-MVP; their existing code and negative evidence are preserved. Rules v11.1.0 /04b606845e0f73ab28b9807bb45682eae4d6ce34; branch dev/ai-enhance-prototype.

Tested implementation source: **25819a9e379a49caaa94df4fa5e2af5112b5cd07**. Source BuildID **7fbe273f5beca507**. Exact final signed bundle/file and source hashes: [historical7fbe evidence](history/EVIDENCE_COLOR_MVP-7fbe.json); raw local evidence ../evidence/2026-10-14-color-25819a9/manifest.json and ../color-evidence. This checkpoint changes documentation only. Local ad-hoc signed development bundle, not a release or accepted production build. No merge/main changes.

Implemented: independent no-model frame/pixel core; official Sony full-range curve and S-Gamut3.Cine-to-linear-Rec709 transform; explicit host sRGB/linear/Gamma2.4; exposure stops, creative Temperature/Tint, signed-log contrast, saturation, optional luminance shoulder; tetrahedral encoded-sRGB WarmFilm v1, LookAmount and linear host-appearance Mix. Native Mac arm64 effect uses external SDK25.6. Frozen look checksum and disk parameter IDs retained. No guessed profile/project gamut. Core remains free of SDK headers; native packaging links only libc++/libSystem.

| Criterion / actual artifact | Status |
| --- | --- |
| Release core/color contracts | PASS3433 +2104 assertions; includes exact strided raw Mix0 bytes/signedzero/NaN payloads, official Sony codes/matrix oracles, finite/extended policy and concurrent renders |
| Final ASan/UBSan | PASS core/color and instrumented native module in owned fixture; host/SDK code not instrumented, TSAN NOT RUN |
| Actual bundle via dlopen in SDK fixture | PASS78 callback assertions: 8/16/32, legacy/SmartFX mapping, ROI/padded rows, integer units, empty worlds, UI/persistent IDs, cancellation/error cleanup and eight parallel frames |
| Full-frame C++ color | PASS9 cases768x320: authored flat SDR and synthetic Sony encoding from3 scenes; max4.30931e-7 vs known reference, tolerance2e-6. This is not real camera Log footage |
| Visual technical review | Root inspected final face/night/rooftop composites at1536x1044; reference restoration matches, WarmFilm remains artistic prototype. Actual flat/camera/motion artistic acceptance NOT RUN |
| Bundle/resource/signature | PASS arm64,exports,actual data-fork PiPL flags/version, strict ad-hoc signature, exact final bytes. No model/ORT/Python dependency |
| Owned installation | PASS added new user MediaCore/FSTR_LUT_Development_7fbe273f5beca507.plugin; verified exact four file hashes. No existing plugin overwritten |
| Actual AE module mapping | PASS25.6x101 PID91632/birth13:58:29 mapped identified installed executable. Registry/effect/render acceptance is a separate open check |
| Initial -noui host attempt | FAIL before test project/report. Menu action entered host error/abort stack; no causal plugin diagnosis. Preserved sample/log. Owned AE/crash handlers stopped after exact identity/nonce checks; absence confirmed |
| Ordinary owned host/UI checks | BLOCKED by locked Mac; startup CLI script execution not confirmed. Real render queue/aerender/8-16-32 output/alpha/ROI/downsample/save-reopen/Undo/MFR NOT RUN |
| Windows / real camera clip / automatic SDK color-space discovery | NOT RUN |
| Acceleration/Fast timing | Deferred after useful color MVP, no new speed claim |

Separate root review against MVP-C01..C10 and senior read-only code/SDK review performed. Closed enum exception-type mismatch, data-fork resource packaging, incremental identity invalidation, finite-domain overflow, very large shoulder cancellation, empty-input format access, unchecked test-result oracle access and exact no-op object-byte copy. The senior did not run our tests. Reviewed boundaries and remaining host assumptions in [NATIVE_REVIEW.md](NATIVE_REVIEW.md).

Continuation ownership: ordinary test AE PID93316, birth2026-10-14 14:08:16, executable /Applications/Adobe After Effects 2025/Adobe After Effects 2025.app/Contents/MacOS/After Effects. Exact process record ../color-host-ui-run/process-snapshot-direct.json; nonce/config ../color-host-ui-run/config.json; protected script owned-test.jsx. CLI session43627. Process was left running because Mac is locked and actual project state cannot be inspected safely. Verify current PID/birth/executable/nonce and project ownership before any mutation or closure; do not kill a user/recovery project.

Next: unlock Mac, use the owned ordinary AE instance/project and File > Scripts > Run Script File if the startup CLI route does not execute the guard. The scenario may only act in its matching nonce-bearing fresh instance. Compare actual outputs to standalone reference with declared integer export tolerances; extend owned tests to linearized semi-transparent edges, save/reopen with nonzero Temperature/Tint, repeated render and MFR. Never forward to/reuse a user AE session or toggle scripting/network/color settings without matching authorization. Camera Log artistic acceptance then requires an identified real source clip. All original v3 obligations remain in PLAN/MVP_COLOR; this milestone is not full MVP completion.

## Historical checkpoints — retained, superseded by the status above

# FSTR-LUT checkpoint — 2026-10-14

## Current priority — overrides historical sequence below

User now requests a color-first MVP: useful grading of flat material «как с логарифмом». All restoration, compression/detail/denoise, training and speed optimization are after MVP. Preserve existing source/research. One essential input/output interpretation is pending in the chat; dependent transform implementation awaits it. See MVP_COLOR.md for acceptance and PLAN.md for authoritative steering history. The native plugin is not yet implemented or installed; current C++ research core is reusable with Enhance0 and no model backend.

Current working branch remains dev/ai-enhance-prototype. Tested color source929b99eb4110ec0b4684ea9120bcc643f7728bec preserves parked optional transfer-table research, compact/student tools and paired quality tools. Nothing was reset/deleted. Exact source/artifact binding: ../evidence/2026-10-14-color-929b99e/manifest.json. This checkpoint update changes docs only. quality-v1 completed20short clip cases plus one public SIDD example; results retained in ../research-block2/quality-v1/quality.json and quality-v1.log. Motion playback QA is NOT RUN. These research results do not block the color-only MVP.

Independent color foundation added in include/fstr/color.hpp / src/color/grade.cpp: explicit linear Rec709/sRGB float-range input, exposure stops, signed-log per-channel contrast and luminance-based saturation. No model dependency, inferred profile or hidden frame state. Not yet wired into frame render/native UI: input/output interpretation is still pending. Core model default is historical research behavior, not a color-MVP default.

PASS: actual Release build and final ASan/UBSan core suite,3427bounded assertions including independent exposure/contrast<1/contrast>1/combined unequal RGB arithmetic, monotonic extended ramp, neutral signed zero, finite/bounded input failures and concurrent pure results. Final sanitizer log ../research-block2/color-kernel-sanitize-final.log. Separate root code review and senior read-only review: input-domain overflow outside float was closed with explicit float-domain validation; output-to-float bounds remain future adapter responsibility.

Rendered fixture fstr_color_fixture executed against final sanitizer build, CSV retains actual negative/above-one linear values. Root inspected1024x432 authored ramp/color preview. Functional arithmetic PASS; artistic acceptance NOT RUN on actual flat footage. Contrast substantially moves black point; full research parameter range is not an accepted UI range. Alpha/frame/pipeline/profile/native tests for the new grade stage NOT RUN (not integrated). No camera-Log/RAW/latitude recovery claim. Fixture-only display clipping is explicit, core does not clamp.

All broader v3 obligations remain in PLAN; restoration and acceleration moved after MVP by direct instruction. Next dependency: clarify desired SDR grading vs camera Log decode vs Log output, then integrate the chosen color transform and frame stage, inspect representative flat-source results and implement the native adapter. Actual AE/Windows/save-reopen/Undo/MFR remain open.

## Historical AI slice — preserved evidence

Contract: USER_SPEC_V3.txt, replacing first brief retained in USER_SPEC.txt/history/PLAN-v1.md. Baseline intake b21b571 preserved. Branch dev/ai-enhance-prototype. Accepted rules v11.1.0 / 04b606845e0f73ab28b9807bb45682eae4d6ce34. Tested core source: d30a40f35091d1a63a7a897d99a6fb8ec832a8ff. Model tooling source be42eb87ed6e0f5b29c82475c0e25c4575fcc667 is unchanged in that candidate. Evidence: ../evidence/2026-10-14-d30a40f/manifest.json, raw check/benchmark records beside it. This checkpoint/report commit is separate from tested source bytes.

## Current result

The first independent research slice exists and runs actual pretrained inference: frame -> C++ FFDNet preparation -> real ONNX CPU model -> tetrahedral LUT -> linear-light blending -> same-size output. Python/PyTorch is not linked into C++. One shared loaded CPU session, serialized inference, reusable packed/output buffers, one exact-content bounded restoration cache. Look/enhance blend changes reuse restoration; input/sigma/dimensions/backend changes invalidate it. JSON research looks embed LUT text and version metadata. Neutral and Warm Film are prototypes, not the final six-look artistic library.

Development only. Stage 1 is still in progress: neither production model nor natural-video quality is accepted. No native AE plugin under v3, no installation, no main merge, no release. Checkpoint does not remove any PLAN requirement.

## Checks actually performed

| Check | Result and exact scope |
| --- | --- |
| C++17 arm64 core + optional ORT CLI build | PASS exit0; CLI SHA256 f0170986bbe546fa9575d7474aa1db35e84114eaebfd395aef5518b10fc6a6db |
| Core contracts | PASS 2377 bounded assertions; actual source tests include stride/bounds, malformed LUT, 1000 seeded parser fuzz inputs, six tetrahedral simplexes, identity17/33/65, extended LUT values, Mix0 exact, Enhance0, explicit colors, premult LUT, cache invalidation/physical capacity and parallel Echo contracts |
| ASan + UBSan | PASS pure core suite; external ORT library not instrumented. TSAN NOT RUN |
| Upstream -> ONNX | PASS FP32 tolerance1e-5, max4.231929779e-6; odd/tiny sizes included |
| Final C++ model + Neutral LUT -> upstream | PASS max3.755092621e-6 on257x255; final binary and fixture hashes recorded |
| Model error contracts | PASS wrong checksum, path traversal, normalization and schema reject with exit1/no output; backend source unchanged since tested be42 |
| CPU -> MPS | PASS research tolerance1e-4 at512x384; FFDNet max2.384185791e-6, NAFNet max6.556510925e-7. No native GPU backend claim |
| CoreML EP | PASS graph placement profile contains CoreML provider and CPU-reference numerical tolerance1e-5. Underlying ANE versus GPU device NOT ESTABLISHED |
| JSON look asset fidelity | PASS embedded cube bytes, checksum, schema/version/color metadata match exported cube |
| Rendered synthetic chart | Inspected at1536x824 and individual512x384 panels. FFDNet cleans noise but softens small text/lines; NAFNet strongly loses synthetic text/stripe detail. This is a counterexample, not acceptance on faces/real footage |
| Natural-video quality/temporal motion/cuts | NOT RUN; five same-input static-scene noise frames do not establish motion or cut correctness |
| Active ORT cancellation/admission wait | NOT RUN; pre-cancel/core behavior tested, monitor ownership reviewed |
| Native AE 8/16/32, SmartFX/ROI/scale/alpha, save/reopen/Undo, real MFR | NOT RUN / not yet implemented under v3 |
| Windows / 4K / security release audit / full six looks | NOT RUN |

## Initial measurements — this Mac only

M1 Pro,16GiB unified RAM, macOS26.6.2. No other agent benchmark/build/test during final serial CLI timing. General desktop load is not controlled. Small samples, no stable performance certification. First frame means first execution in the created process/session, not a clean OS/device-cache cold start.

| Path | Input / settings | Observed result |
| --- | --- | --- |
| FFDNet PyTorch MPS full reference | Synthetic1920x1080,FP32,sigma15/255; includes upload/download | First257ms; warmed219ms (~4.57fps) |
| NAFNet PyTorch MPS | Same1920x1080 input | First3587ms; warmed2560ms (~0.39fps). Earlier trial OOM at same40% allocation policy; later trial passed. Memory stability not accepted |
| FFDNet ORT CoreML trunk | Fixed1920x1080,excludes C++ color/LUT/AE | Warm median239ms over3 runs; CPU max error1.132488251e-6 |
| FFDNet C++ ORT CPU full pipeline |1920x1080,32bpc,Enhance0.5,Warm Film1.0,4threads,cache disabled | Three runs3718/3815/3537ms total; peak child RSS940769280bytes (~897MiB). Includes core prep/model/LUT/mix; excludes AE/file reading/model session load |

**Fast33–40ms goal: FAIL for measured paths.** Do not connect an aggressive default automatic effect and call it ready. On the paired512x384 synthetic Gaussian test, input24.72dB, ordinary blur26.87dB, FFDNet32.66dB, NAFNet22.37dB. Both change clean synthetic input; PSNR is not a substitute for natural video review. Results cannot be generalized to real-noise model training domains.

## Separate final review

Spec review: first research slice matches explicit architecture/measurement priority. Full product remains incomplete; PLAN traces all postponed alpha/extended AI, temporal/cut/stable auto-color, skin protection, full library, presets/native portability, Fast/Best, AE/Windows and memory/performance requirements. No fake advanced controls or successful restoration fallback. Custom cube UI remains after MVP.

Quality review: root reviewed frame arithmetic, parser bounds, LUT simplex sampling, transfer/mix units, cache keys/capacity, immutable render settings, session/buffer ownership, exception boundary and cancellation paths. Authorized senior model gave independent source advice/review; read-only, no edits/tests claimed by that reviewer. Corrections: borrowed Ort TypeInfo lifetime; nonallocating error-message fallback inside noexcept; padded area budget; bounded line reading; physical cache capacity; explicit thread error propagation; color oracle coverage; removal of potential per-pixel stable_sort allocation. Rechecked affected final core/sanitizer/parity scenarios. Fault injection and active ORT cancellation remain unverified.

Initial failures/history are retained in the evidence manifest: export3e-6 threshold, faulty h1 stride oracle, initial CLI exit139 (precise causal reproduction NOT RUN), first NAFNet OOM, and denied sysctl inside time wrapper. No unsafe memory limit was removed; final timing/RSS used a serial subprocess and resource API.

## Reproduce

Pure core: `python3 scripts/check.py`; sanitizers: `python3 scripts/check.py --build build-sanitize --sanitize`. Optional CLI: configure CMake with `-DFSTR_ORT_ROOT=/path/to/onnxruntime-osx-arm64-1.23.2`, then build.

Research preparation (external pinned KAIR/source and official checkpoint required; hashes enforced): `python tools/model_export/export_ffdnet.py --source /path/to/KAIR --weights /path/to/ffdnet_color_clip.pth --output /new/research/export`. Exact model manifest is in assets/models; external model bytes are not committed. Root lockfile tools/model_export/requirements-lock-macos-arm64.txt captures this research environment, not cross-platform plugin dependencies.

Compare: `python tools/benchmark/compare_models.py --research /path/to/research --output /new/research/results --device mps --full-hd` (requires pinned KAIR and NAFNet folders plus models). CLI usage is printed by fstr_benchmark. All prototype processing is local; no render-time network.

## Next concrete work

Finish Stage1: compare a lighter same-resolution model/weights and clean-material adaptive bypass on a rights-cleared representative video corpus; confirm actual redistribution rights. Keep independent frame/short-video quality, speed and memory gates. Only then implement the minimal AE adapter and measure its complete path. Do not fix current latency by silently changing resolution, removing memory guards or asserting untested tiling equivalence.

Cleanup: only owned intermediate/build/research directories were created. They are retained for reproducibility and rollback; no uncertain material, installed plugin, project, other checkout or historical Evidence was deleted.
