## Smart input preparation reuse — 2026-10-18

**SMART-INPUT-PREPARATION-001 implemented, verified and installed1c490d4515b2937b.** AE input-only state equality + explicit frame/pre-look/format/geometry context reuses the unchanged bounded image and full SHA;64entry/32MiB resident RAM only, no disk/model/color changes. Legacy/unavailable-state fallback, cancellation/clear/concurrency tested.22Release/22ASan+UBSan +targetedTSan pass. Actual paired warm960x540/32bpc median1251→751ms (39.97%less elapsed,3pairs),15full image/hash oracles exact.51successful exports across partial/retry/canonical jobs;23accepted-baseline and4actual paired comparisons <=1exportcode. Upstream/manual/16bpc invalidate; LUT/strength/re-enable reuse. Signed ed9 and accepted42ee preserved; own sessions/helpers cleaned up. Separate CoreML raw-model experiment2.687x median Run on6inputs with small numerical differences; full masks/window/AE/device/loading validation remains NOT_RUN and product remains CPU. [Scope, failed harness attempts and next step](SMART_INPUT_PREPARATION.md). No installer/release.

## Smart color evaluation reuse — 2026-10-18

**SMART-COLOR-REUSE-001 implemented, verified and installeded9e33d687019dc1.** Reuses one float-rounded ordinary LUT result inside the Smart residual, including people/stable paths and AE plain subtraction; primary ColorPass stays separate. Frozen legacy276480result/error records/46080rows/18look sources are bit-exact (7004160bytes).20Release/20ASan+UBSan pass, targeted evaluation/binding/bypass tests and separate code review complete. Residual-only CPU benchmark3.23..4.99x; interleaved actual AE warm-frame pairs median1764→1238ms (29.82%less elapsed time,960x540/32bpc,3pairs).27actual exports:16accepted42ee baseline comparisons +4old/new paired comparisons <=1code. Canonical installed build verified separately; raw mask persistence and5RAM hits per LUT/strength edit retained with zero inference. Both own AE sessions/helpers cleaned up; prior7be and original accepted42ee preserved signed/exact. Model/hash/preparation costs and real-time playback remain separate. [Contract, exact scope and evidence](SMART_COLOR_REUSE.md). No installer/release.

## Smart cache latency — 2026-10-18

**SMART-CACHE-LATENCY-001 implemented, verified and installed7be976a9a5160b0e.** Short resident lock + serialized miss gate; same-key recheck, cancellation/reentry/clear-generation handling,8entry/unchanged128MiB RAM ceiling retain a5-frame window. Real warm hit during another real inference2.287ms versus prior2114.139ms; model policy unchanged.19Release/19ASan+UBSan,2targetedTSan plus queued-clear affected reruns pass.24actual AEexports across trial/canonical,18accepted-baseline comparisons <=1exportcode; LUT/strength edits use5RAM hits each with zero disk/inference. Canonical replay reads24raw disk entries with zero inference. Own sessions/helpers cleaned up; new Current signed/inventory-verified, accepted42ee archived exact as quality reference. Color/model/hashing work and production playback FPS are not accelerated/qualified by this cache step. [Contract/evidence/next step](SMART_CACHE_LATENCY.md). No installer/release.

## Smart stage profiling — 2026-10-18

**SMART-STAGE-PROFILE-001 complete for the measured paths; no product change.** Preserved accepted installed42ee1edaaff01753. Six raw analysis comparisons match exactly. Scene model Run accounts for76.22% of standalone raw-analysis time; initialized repeated analysis median2302.791ms. Resident hit2.382ms waits2114.139ms behind an unrelated real miss. Actual AE diagnostic16exports/13baseline comparisons <=1exportcode; no inference rerun on toggles/strength edits. Real RAM-warm render1509ms: color973.296ms, upstream hashing199.858ms, raw cache7.172ms. The4-entry RAM ceiling cannot retain a5-frame middle-of-shot window, measured as5disk reads. Both own AE sessions exited; diagnostic effect/helpers archived and accepted inventory/signature unchanged. Next: short RAM locking + preserved miss serialization/window retention, then redundant color/hash work, then measured scene-model acceleration. Playback FPS and optimized product behavior remain NOT_RUN. [Method, results and limits](SMART_STAGE_PROFILING.md).

## Continuous Smart across cuts and ordinary preview — 2026-10-18

**SMART-CONTINUOUS-CUTS-001 shown quality accepted by user («качество принимаю»); artifact/runtime checks complete.** Unchanged installed42ee1edaaff01753 exports343frames:168OFF/168ON, toggle/repeats; OFF core and repeat/earlier-isolated comparisons <=1exportcode, Smart effects exceed dither-only differences. No new model inference on toggle/repeats or ordinary cached UI preview. Full/cropped14s movies decode; sampled stills and worst motion-diagnostic pairs inspected with no observed defect. User accepts shown quality; exact viewing coverage is unspecified.42ee is the preserved quality baseline for acceleration. Actual Space preview starts, advances and stops; steady playback FPS not measured. First ON pass is slow:462.341s for7s of footage (observed workflow timing with partial existing cache/concurrent reference work, not isolated benchmark). Own project saved, own AE exited normally/helper archived, installed signature/inventory unchanged. [Outputs, checks and explicit limits](SMART_CONTINUOUS_CUTS.md). No product-code change, installer or release.

## Scene-aware SDR preparation — 2026-10-18

**SDR-SHOTS-001 implemented and installed42ee1edaaff01753.** Fresh effects enable Advanced → Detect Scene Changes; old saved projects initialize itOff. Complete bounded scan detects cuts and selects separate immutable SDR recipes. Exposure changes/returning flashes/gradual ambiguity are handled conservatively; Smart neighbor recovery is restricted to the current shot. Fixed a real AE render-clone cache defect: identical upstream input states now share a bounded32-entry content cache, with zero repeated timeline scans across the168-frame warm sequence or LUT-strength edits. Upstream and bitdepth changes invalidate it.18Release/18ASan+UBSan plus focused failure/cancellation reruns;187actual AE exports,168core comparisons, legacy/depth/bypass/Smart-cut/save-reopen/fresh-disk/ROI checks pass in scoped tolerances. Video prepared and sampled stills inspected; continuous new-clip human acceptance and broad cut/interlaced/retiming/real-time coverage remain separate. Original492 and diagnostics preserved; own helpers archived. [Contract, exact checks and limits](SDR_SHOT_DETECTION.md). No installer/release.

## Current-default SDR comparison — 2026-10-18

**SDR-DEFAULT-REVIEW-001 complete for artifact/technical/sample-frame scope.** Unchanged installed49235442fbda89fd exports496frames across three real edited-SDR scenes plus an explicitly artificial-flat control. Four playable source/output comparisons, four-stage boards, motion crops and saved/reopened AE project are ready. All480processed rounded outputs agree with independent core references within one8bit code; separate float diagnostic maximum1.001368codes is retained. Default Natural and Auto-only are visually equivalent; no manual grading is hidden. Sampled faces/white fabric/shadows/clothes inspected without an observed product defect; no algorithm/default changes. User reports no visible flicker in the shown comparison clips; scoped user visual flicker acceptance is recorded. Other footage, SmartOn and broad artistic/Log-equivalence acceptance are not inferred. See [review, outputs and limits](SDR_DEFAULT_REVIEW.md). Own helper archived; installed bytes unchanged; no installer/release.

## External LUT native matrix and reload/Undo fixes — 2026-10-18

Installed signed **49235442fbda89fd**. Named Custom retains saved slot18 and filename; separate final **Load LUT...** action19 supports reliable repeated loading. No separate button or filename block. Human GUI Undo is verified onf2c3a3d598d8e15a: standard Undo restores the previous filename and rendered pixels. Removed redundant FORCE_RERENDER as directed by the SDK. Final492 changes the malformed-file failure path: return a user notification and commit restoration to the previous Look, preventing a transient19 from breaking subsequent renders.

Actual AE fixture imports3DL/IRIDAS.look/cineSpace.csp, Cube/Undo, cancel, missing-file/relocated-path/save/reopen and malformed-file→next valid import pass across identified builds; independent decoded pixel comparisons <=1/255+1e-6. Final492 specifically verifies error notification, callback0, retained Look18, prior pixels, save/reopen and next valid Cube import. Focused Release+ASan/UBSan callback checks pass on final492; prior17+17 full-suite results belong to521d. See [native matrix, artifacts and exact limits](LUT_NATIVE_MATRIX.md). No installer/release or general artistic acceptance.

## Custom LUT menu and Auto-first UI — 2026-10-18

Installed **521d20d5d788a661**: no import button/LUT-file block; final Look item opens chooser, then shows Custom: filename. Auto Prepare SDR first/defaultOn for fresh effects; old projects retain saved values and explicit Log/3D skips SDR preparation. Corrected hidden button type compatibility and popup encoding; non-ASCII names display Latin transliteration while references retain UTF-8.17Release/17ASan+UBSan and focused callback reruns pass. Actual AE Cube import/render/save-reopen/strength/Mix/Smart reuse, fresh-process missing/recovery, old-project pixels, fresh default and final UI inspected. Other-format native GUI and full undo/cancel/relink matrix remain NOT_RUN; see [EXTERNAL_LUT.md](EXTERNAL_LUT.md). No installer/release.

## External LUT import — 2026-10-18

**IMPLEMENTED / AE HOST BLOCKED.** Last Look entry **Custom**, Import LUT chooser and filename; native four-format OCIO2.4.2 backend (.cube/.3dl/IRIDAS.look/cineSpace.csp), reference-only project serialization, explicit missing-file behavior, immutable render processors and compatible Smart-cache reuse are implemented. Existing IDs/defaults preserved.17Release +17ASan/UBSan tests pass; scoped code scanner reports no findings. Actual chooser/save/reopen/undo/cache behavior inside AE is NOT_RUN: own empty AE pid24652 stalls before plugin/helper load in Adobe previous-session `sysctl`; SIGTERM/SIGKILL do not complete. Installed build restored byte-exact to previously host-verified **ac8fd7cefc63305c**. Candidate **fa7cd7bd1a3291eb** and diagnostics retained separately. Do not describe external import as installed/host accepted. [Contract and resume point](EXTERNAL_LUT.md).

## FSTR Grading — product name, 2026-10-18

User selects **FSTR Grading** in **FSTR Effects**. Display/category/Info/About updated; internal match names/indices/cache paths preserved. Actual AE registry, fresh effect, old-project pixel/cache reuse and normal own exit pass. Signed candidate **ac8fd7cefc63305c** installed; previous1eebb7fa77c9f1a0 archived intact.16Release tests pass. [Evidence](PRODUCT_NAME.md).

## Optional native SDR preparation — 2026-10-18

SDR-PREP-AE-001 implements **Auto Prepare SDR** as an appended, defaultOff native control. Fixed first/middle/last clip anchors choose one immutable SDR recipe before manual grading/LUT/Smart. Off/Mix0 skip anchor analysis; Mix includes preparation. Compatible recipes use bounded64-entry RAM /2048-entry8MiB versioned disk storage; algorithm, pixel content, geometry, transfer and fixed timing identify entries. Existing parameter indices/defaults remain unchanged. Single-shot clips only; no automatic segmentation, Log inference or recovered SDR latitude. Corrected candidate **1eebb7fa77c9f1a0** is installed. Actual AE163exports cover72base frames individually within one8bit code of core references,72Smart frames, switches/old-project/save-reopen and fresh-process1recipe+3mask disk hits with0first-frame computes. Found/fixed AE output-time cache alias; previous failed candidate retained.16Release/16ASan+UBSan targets pass. Continuous new-clip visual acceptance remains separate. See [native contract/checkpoint](SDR_PREPARATION_AE.md).

## Automatic SDR preparation prototype — 2026-10-18

SDR-PREP-001 adds a standalone C++17 shot-anchor estimator and bounded SDR tone/chroma transform, versioned v2 CLI recipe and meaningful numeric/behavior tests. White-uniform defect in first median estimator corrected by dark/light-anchor fitting; failing regression retained.3Release/3ASan+UBSan focused targets and6CLI refusals pass.240base+240styled frames, byte-exact out-of-order repeat, three decoded comparison videos reviewed in sampled stills. This is separate core research: installed Current9838bb4d38c6e9c9 unchanged; AE controls/shot selection/recipe-cache integration pending. No universal professional/Log-equivalent or continuous flicker acceptance. [Contract/evidence](SDR_PREPARATION.md).

## Three-scene SDR grading examples — 2026-10-18

SDR-GRADE-REVIEW-001: two additional real outdoor shots,194accepted actual AE exports and eight core comparisons within one8bit code.192CPU float outputs finite/in-range with unchanged alpha. Delivered Natural and Teal&Orange65%+Smart comparisons covering kitchen/performers/wide group,14s each, plus a saved/reopened editable AE project. Final visual review rejected an existing author dissolve; replaced84–86s with clean82–84s and repeated affected checks. No product math/default/model changes; installed Current9838bb4d38c6e9c9 preserved. Fixed manual shot settings and sampled visual judgments do not establish universal automatic/Log-equivalent grading. [Outputs/evidence](SDR_GRADE_REVIEW.md).

## Short-clip Smart cache retention — 2026-10-18

CACHE-RETENTION-001 fixes the64-frame disk cap that evicted part of the72-frame real-SDR trial below its byte budget. Disk now retains up to4096entries/512MiB; RAM/keys/models/color formulas unchanged.14Release/14sanitizer targets plus extended capacity tests pass. Actual AE Current9838bb4d38c6e9c9 exports217frames: only8previously evicted frames compute during warmup; SmartOff makes no accesses; re-enable and strength edits perform zero new computes across72frames each. Repeated pixels within one8bit code; prior Current preserved and helper archived after normal exit. This is cache behavior, not artistic or real-time acceptance. No installer. [Contract/evidence](CACHE_RETENTION.md).

## Real SDR motion sample — 2026-10-18

User delegated source finding. Official DPReview Nikon Z6 Flat-capture reel acquired; intro acknowledges FinalCut tweaks, so not an untouched/ungraded camera original. Real20–23s kitchen segment has two people/steam/white uniforms. Unchanged Current923695974aa8fc72 renders146actual AE exports; numerical references within one8bit code; both72-frame sequences and two9scomparison videos decode.64retained analysis entries each describe2people. Sampled full-size/crop visual review performed; continuous flicker/professional artistic acceptance remains open. First Smart export is slow; no real-time claim. No new product build or installer. [Source, checks, limits](REAL_FLAT_SDR_TRIAL.md).

## Natural Contrast — 2026-10-18

Shared luminance contrast now defaultsOn in fresh/reset AE effects; old projects initializeOff through the SDK compatibility flag and retain exact prior renders.14Release/14sanitizer targets and11actual host exports pass, plus keys/expression/save-reopen checks. Smart toning-mode change:2computes/7RAMhits. Current **923695974aa8fc72** installed, prior bytes archived. Control comparison is explicitly artificially flattened SDR; actual flat-SDR artistic acceptance and Advanced-checkbox visual inspection remain open. No installer. [Contract/evidence](NATURAL_CONTRAST.md).

## Real EXR workflow and input help — 2026-10-18

Official OpenEXR Blobbies CG reference explicitly converted from its custom primaries into two known spaces. Four actual AE linear-project exports (Linear RGB/ACEScg × Natural/Teal&Orange) agree within one8bit code; unmanaged import negative control retained.14Release/14sanitizer targets pass. Simple source guidance/About updated; actual help typography inspection NOT_RUN (AE screenshot unavailable). Current **2c6caa579d3fa42f** installed; prior bytes archived. No installer. Professional-looking flat-SDR outcome remains **not accepted** by these technical checks; C2098 side-by-side is only a manual example. [Scope/evidence](EXR_HELP.md).

## Dynamic flag error fix — 2026-10-18

Fixed AE25::228 by preserving host flag words during PF_Cmd_QUERY_DYNAMIC_FLAGS.14Release/14ASan+UBSan targets pass; regression fails on previous code. Actual AE completes709queries, eight exports, Smart switches, save/reopen and normal exit. Five old/new comparisons exact; repeated Smart-on within one8bit code. Current **9177edacb182d7a5** installed, prior bytes archived. See [DYNAMIC_FLAGS_FIX.md](DYNAMIC_FLAGS_FIX.md).

## 3D Linear input — 2026-10-18

Added **3D - Linear RGB** beside ACEScg, moved Input Profile inside Advanced and preserved sRGB/Already converted defaults.14Release/14sanitizer targets,718prior profile vectors, eight exact legacy host comparisons, animated/name-expression migration and save/reopen pass. Six actual32bpc AE frame checkouts retain distinct above-white values with/without a look. Signed Current **f6153fc453e7167e** installed; prior Current archived. Unknown Log or arbitrary renderer gamut is not inferred from bit depth. OpenEXR file export NOT_RUN; actual host float receipts used instead. [Contract/evidence](LINEAR_3D_INPUT.md).

## Current input extension — INPUT-PROFILES-001 / META-XMP-001

Five explicit sources append to Host/Sony: ARRI LogC3 EI800/AWG3, LogC4/AWG4, Panasonic V-Log/V-Gamut, ACEScct/AP1, ACEScg/AP1.14Release/14sanitizer targets,718OCIO vectors and nine actual AE exports pass; old Host/Sony results match exactly. Generic bounded XMP TIFF manufacturer/model reader added for MP4/MOV; it does not infer gamma/gamut. Six synthetic vendor-name contracts, a decodable authored MP4 and two real Sony regressions pass. Untouched non-Sony layouts and remaining vendor profiles stay pending. New Current build8d3e3dda483dffcf installed with prior bytes preserved. See INPUT_PROFILES.md.

## Current next slice — CACHE-RECOVERY-001

User accepts Smart checks on installed build d774807231370f48. Preserve its algorithm; versioned interrupted-write cleanup with cross-process live-writer exclusion is implemented and passes14Release/14ASan+UBSan targets plus the scoped six-render AE test. See CACHE_RECOVERY.md. This supersedes the pending user-trial acceptance in older entries below.

## Active follow-up — automatic Smart, 2026-10-18

SRGB-DEFAULT-001 replaces the Auto follow-up: user requires sRGB default and manual Advanced choice, and explicitly cancels Auto. Preserve saved ID9values1–4and verify old-project/default migration plus new-effect rendering. Auto bridge/converter investigations are historical and not delivery prerequisites. See COLOR_AUTO_FOLLOWUP.md.

Automatic native analysis/cache has actual AE trial evidence for switches, cut, motion, save/reopen and fresh-process reuse. Corrected-bundle load/switch checks also pass. Full motion/restart replay on that exact bundle, new-port continuous perception, wider workflow acceptance remain open; sRGB-default migration now passes its scoped host checks; the16-stage swatch method is rejected by actual host evidence. [Acceptance and sequence](AUTOMATIC_SMART.md). Earlier obligations and evidence retained below.

# Historical plan checkpoints

Prior Auto obligations below are superseded by SRGB-DEFAULT-001. Current encoding acceptance:14Release +14sanitizer checks and actual AE old/new/save/reopen output comparison PASS; see SRGB_DEFAULT_EVIDENCE.json.

# FSTR-LUT — AI Enhance + Looks v3

## Scalar toning in AE — host slice verified, 2026-10-18

Native stable-strength core and schema2read-only analysis bridge now render in isolated AE25.6. Fixed the measured GUID-protocol export regression.77image-reference checks,3switch/upstream checks, fresh-process disk read and unseen-shot hard-cut fallback pass;12release +12sanitizer targets pass. This is prepared-clip integration: arbitrary-media native analysis, time-remap invalidation, Auto encoding and continuous visual acceptance remain open. [Exact host/artifact scope and evidence](SCALAR_NATIVE_INTEGRATION.md).

## Scalar Smart strength averaging — 2026-10-18

Implemented per-person anchor strength interpolation using current-frame native grading and geometry.36frame face-color motion diagnostic improves10.50%; same-geometry raw-strength ablation improves11.08%.4new schedule tests pass. Native AE integration and continuous visual acceptance remain open. [Algorithm, checks and limits](REFERENCE_CORRECTION.md#follow-up-scalar-strength-current-frame-geometry--2026-10-18).

## Reference-frame averaging — 2026-10-18

Added and tested explicit interpolation of added color correction between automatic per-person anchors.5math/policy tests and20existing analysis tests pass.36frame trial worsens measured face color motion42.10%; not promoted to AE. [Contract, result and checkpoint](REFERENCE_CORRECTION.md). Prior native bridge work remains preserved; host/Auto/flicker acceptance open.

## Automatic individual people — 2026-10-18

Standalone automatic3-person/36frame combined scene+individual tone trial passes exact Off/reverse checks;14Python and11sanitizer C++ checks pass. [Implementation, evidence and open native/visual work](INDIVIDUAL_PEOPLE.md). AE integration and Auto encoding remain incomplete.

## Current color follow-up — 2026-10-17

Manual encoding moved to collapsed Advanced in candidate source; Auto freshness unresolved and host UI/migration unverified. Individual per-person skin-tone adaptation now has an independent experimental core and scene-core hook; automatic association/video/AE still open. [Implementation and checks](INDIVIDUAL_SKIN_TONE.md). [Requirements and remaining checks](COLOR_AUTO_FOLLOWUP.md).

## Current Smart completion block — 2026-10-17

Native Vision RAM/disk cache passes13real AE renders including restart; original effect preserved, separate Cache Test installed. CPU SAM2 reference repeats29/29predictions exactly. New36frame/3face/partial-occlusion trial and two real cuts evaluated; soft proposal edges and interval fades added. Skin-field bands remain visible; continuous flicker acceptance and new whole-scene/SAM2 native backend are still open. **Full requested block is not complete.** [Requirement matrix and defects](SMART_COMPLETION_BLOCK.md), [evidence](SMART_COMPLETION_BLOCK_EVIDENCE.json), [native cache](SMART_ANALYSIS_CACHE.md).


## MASK-CACHE-001 — agreed addition, 2026-10-17 (native Vision slice implemented; whole-scene extension pending)

User explicitly requested remembering this for later implementation. Maintain an analysis cache independent of Smart On/Off, look choice and look intensity. Smart Off skips analysis while retaining existing masks; On reuses compatible cached masks. LUT changes regrade without repeating detection/propagation. Invalidate affected entries when upstream pixels/time mapping, input interpretation or analysis/model settings/version change. Bound memory use; add a versioned validated disk cache for reuse across AE sessions. Concurrent requests must share in-flight analysis where safe; cancellation and cache failure must not yield false successful analysis. Acceptance must verify detector/propagation invocation counts through On→Off→On and LUT changes, plus stale-cache rejection and memory/disk limits. Native Vision observation caching is implemented and passes isolated AE32bpc tests including restart; whole-scene/SAM2 tensor caching and the new native analysis backend remain pending. See SMART_ANALYSIS_CACHE.md.


## Shot analysis reliability follow-up — 2026-10-17

Added two-sided endpoint validation and exact rejection of unreliable flow.26contracts pass. Two fixed-setting independent MPS runs remain non-bit-exact; actual native color differs by>1/255 on at most12pixels/frame on this16frame trial. Repeatability mechanism, independent multi-person/occlusion footage and continuous perception remain open. No AE promotion. [Measured evidence](SMART_VIDEO_RELIABILITY_EVIDENCE.json).


## Offline semantic + SAM2 + motion comparison

Automatic skin anchors and independent SAM2 directions now feed a bounded hybrid with optional reliable optical flow. The16frame difficult-arm trial improves mask strength stability; flow adds no meaningful gain here.17tests pass,64native comparison renders preserve the16frame baseline exactly. No new AE installation. Full-clip/multi-object quality, perceptual flicker, and inference repeatability remain open. [Result and next boundary](SMART_VIDEO_ANALYSIS.md), [exact evidence](SMART_VIDEO_ANALYSIS_EVIDENCE.json).


## Combined scene + people — standalone prototype, not promoted to AE

Added explicit local UperNet candidate (model-card MIT), independent existing pose/face/skin evidence, source-pixel detail bands and bounded probability fusion. Known test_face recovered;166cases including156consecutive frames render with exact Off and reverse-order equality.24scene +40person +15adaptive tests, face/turn regressions and9native checks pass. Skin mask motion MAE drops19.6%within this path; arm confidence still fluctuates under strong blur. Continuous perceptual approval, crowds/preview parity and new AE integration NOT RUN. Installed plugin preserved. [Combined path and limits](SMART_SCENE_PEOPLE.md), [evidence](SMART_SCENE_PEOPLE_EVIDENCE.json).

Next: reduce the verified arm confidence fluctuation around frames144–152 without forcing unsupported long gaps; validate mixed objects/occlusion and continuous motion before native export or stronger look tuning. Broader camera/metadata obligations remain below.


## Whole-scene Smart — standalone slice implemented, not promoted to AE

C++ TealOrange residual + automatic scene/person-part probabilities + soft spatial/explicit temporal fusion implemented.166case run includes156consecutive user frames;9release/9sanitizer/9Python checks pass. Motion-compensated mask error drops16.8%overall/18.7%skin; continuous perceptual approval NOT RUN. Fixed uniform-field false semantic confidence, final6case strict run passes inputs/provenance/negative controls. Complex-light skin and object misclassification remain FAIL; research-only model cannot ship. Existing installed plugin unchanged. [Implementation](SMART_SCENE_IMPLEMENTATION.md), [evidence](SMART_SCENE_EVIDENCE.json).


## Smart grading of the entire scene — revised scope, research complete

User requires automatic preset-directed toning of all objects/materials, with skin as one part; original-hue protection is superseded. Professional-source review rejects fixed skin warmth as a sufficient Smart implementation. Preserve light/material relationships, soft confidence-weighted regional adjustments and temporal coherence. User reports small-face hard boundaries and mask toggling: visual acceptance remains FAIL. Unvalidated paired-grade draft archived outside product; runtime unchanged. [Research, revised contract and next experiment](SMART_SCENE_GRADING.md). No new host/model/visual PASS.


## Actual AE Smart trial — local functional checks passed

AE25.6 loaded the isolated trial, saved/reopened Smart state and rendered8/16/32bpc. Short50recovers; long54–56falls back to plain LUT. ROI exact;156frame comparison exported; nine-frame MFR-enabled aerender matches. OriginalMP4 render passes at resolved path. Trial remains separately installed; original/userAE preserved, helper archived. Continuous perceptual approval, old-project migration and Undo remain NOT RUN. [Review](SMART_AE_HOST.md), [evidence](SMART_AE_HOST_EVIDENCE.json).


## Clarified long-gap fallback — ordinary LUT, short interpolation retained

User narrowed fallback to complex long gaps. Cancel further forced long-gap tracking; preserve verified1–2frame interpolation. Actual module54/55/56 equals Smart Off byte-for-byte; short50still recovers504pixels. Existing runtime already conforms; regression and contract updated, no algorithm change. Continuous transition and actual AE remain NOT RUN. [Contract/evidence](LONG_GAP_FALLBACK.md), [hashes](LONG_GAP_FALLBACK_EVIDENCE.json).


## Native two-sided interpolation — bounded implementation

SmartFX now uses explicit ±2frames to bridge at most two missing detections. Coarse color signatures and bounded position search must match both raw anchors; ambiguous/intersecting intermediate detections reject. Actual156frame replay restores50/116 only; real module frame50 confirms504changed pixels. Eight release and sanitizer targets pass. Long gaps54–56, complete motion quality and actual AE remain open. [Contract/results](NATIVE_REGION_INTERPOLATION.md), [evidence](NATIVE_REGION_INTERPOLATION_EVIDENCE.json). Installed plugin unchanged.


## Smart Color Grading On/Off — native candidate, not installed

Checkbox defaultOff/diskID13, real macOS Vision regional protection, soft masks, bounded adjacent-frame geometry, full-input SmartFX and cancellation/error cleanup implemented. Eight release and sanitizer test targets pass; real module On changes563pixels on frame48 and Off restores plain LUT exactly. Native detector finds regions in78/156frames and misses visible blurred faces54–56: full Smart quality NOT ACCEPTED, gap/body fallback not yet ported. Actual AE/continuous perception NOT RUN. [Contract and checks](SMART_COLOR_NATIVE.md), [evidence](SMART_COLOR_NATIVE_EVIDENCE.json).

Appearance research now permits same-colored props inside regions and uses soft blobs for blurred heads.156frame reverse-order equality passes, but added-color instability mean slightly increases despite lower mask MAE. Flicker remains open; see [motion review](COLOR_REGION_MOTION.md). Earlier semantic failures remain historical evidence under their original contract.


## Dynamic coarse-to-detailed regions — implemented research slice

User accepted approximate heads for wide shots and requested dynamic detail during approach/recede. Implemented source-height bands, independent face observations, bounded size smoothing and short coarse-box recovery. Validate actual camera approach/recede and crowded crossings, continuous perceptual motion and remaining detailed skin occlusion before AE promotion. Do not replace these checks with synthetic zoom or unit success. [Review](DYNAMIC_FACE_REGIONS.md), [evidence](DYNAMIC_FACE_REGIONS_EVIDENCE.json).

## Automatic multi-person protection — next step

No manual selection. Implement independent face tracks with optional body/head support; mandatory body confirmation failed group/close-up controls. Isolate ambiguous tracks, preserve global hand veto and bounded deterministic windows. Validate crossings, entry/exit, overlapping hands and many small faces. Semantic occlusion and continuous flicker remain open. [Review](PERSON_HEAD_AUTOMATION.md), [evidence](PERSON_HEAD_AUTOMATION_EVIDENCE.json).

## Broader input color interpretation — user request2026-10-15

Support other camera Log/gamut pairs and non-camera encoded sources, not metadata alone. Scope candidates include ARRI LogC, Canon Log, Panasonic V-Log, Nikon N-Log, Fujifilm F-Log, Blackmagic Film, RED Log3G10, DJI D-Log, Apple Log, Cineon, ACEScc/ACEScct and linear CG/interchange sources. Treat each actual profile/version and gamut explicitly; names alone do not define transforms. PQ/HLG are separate HDR transfer functions, not camera Log. This list is a planning scope, not a claim of present support or verified official formulas.

Before implementing each transform, use authoritative current definitions/test vectors, explicit code range/primaries/whitepoint, correct linear working conversion and documented display/output handling. Validate neutral/color/range/roundtrip cases and representative real exports. Metadata suggestion must refer to incoming pixels, never silently override already-transformed host footage. Preserve manual selection, legacy parameter IDs/state and existing project appearance. First five additional sources now implemented under INPUT-PROFILES-001; other listed profiles remain pending.

## Multi-vendor metadata follow-up — user request2026-10-15

Extend read-only camera metadata recognition beyond Sony. Candidate coverage: Canon, Panasonic, Nikon, Fujifilm, Blackmagic and ARRI, conditional on documented accessible file metadata and real fixtures. Preserve separate manufacturer/model, gamma and gamut fields, provenance/conflict handling, manual input interpretation and no inferred profile from image appearance or generic Rec709 tags. Parsing metadata does not automatically add pixel-transform support for every camera Log/gamut or authorize silently changing already-converted AE pixels. Validate missing/conflicting metadata and representative real files per vendor. Generic standard XMP names now implemented under META-XMP-001; proprietary vendor gamma/gamut layouts and untouched non-Sony footage remain pending.

## Skin-colored occlusion acceptance — 2026-10-15

User requested closure of occlusion and continuous flicker.84 synthetic cases and actual FaceOcc planted-patch counterexamples expose remaining semantic failure. Correspondence guard is limited to temporal additions. Scope/results: [SKIN_OCCLUSION_ACCEPTANCE.md](SKIN_OCCLUSION_ACCEPTANCE.md). Next candidate must address semantic occluder rejection; no universal temporal veto or AE promotion on this evidence. Prior obligations retained.

## Blur recovery follow-up — 2026-10-15

User authorized motion-aware mask recovery and conditional directional softening. BR01..05 scope and exact results: [BLUR_RECOVERY.md](BLUR_RECOVERY.md). Known55 dropout improved; overall motion/occlusion/flicker acceptance remains open. All earlier obligations retained.

## Occlusion follow-up — 2026-10-15

User authorized visible-face/hand/prop separation after turn recovery. ScopeOC01..05: [OCCLUSION_REVIEW.md](OCCLUSION_REVIEW.md). Independent pinned FaceOcc gate; compare face retention and rejected occluder separately. No automatic promotion to AE; repeat full supplied sequence and prior23scene cases, preserve failed candidates. Continuous playback and prior acceptance remain required.


## Turn recovery continuation — 2026-10-15

User authorized implementation of short-gap mask transport, profile support and confidence transitions. Scope/acceptance TR01..06: [TURN_RECOVERY.md](TURN_RECOVERY.md). Preserve prior candidate/defaults/evidence; research remains outside AE. Actual short-gap recovery must use raw mesh-confirmed two-sided seeds and current evidence, with explicit radius2 and no recursive neighbors. Profile fallback is separately measured and cannot become a recovery seed. Quality and continuous playback acceptance remain mandatory; no integration/release implied.

## Face-supported follow-up — 2026-10-15

User explicitly requested stronger skin selection including face confirmation and complete visual flicker inspection. Implemented independent face proposals + mesh confirmation + semantic skin intersection; real720×1280 analysis, same360×640 native comparison. Tested on all prior known errors and all156 supplied-video frames. [FACE_SKIN_REVIEW.md](FACE_SKIN_REVIEW.md) records exact coverage and failures. Architecture/background false positives improve, but profile/edge misses and abrupt protection dropouts prevent quality acceptance. Every-frame overview/detail review is complete; continuous25fps perceptual playback and complete native-scale whole-frame review are NOT RUN. Full flicker acceptance is not closed. No new plugin installation, release or publication.

Next bounded quality work: address uncertain detection transitions without painting occluders/background, validate unseen scenes and motion/cut/occlusion cases, then actual continuous playback and remaining host gates. Do not substitute weaker acceptance or disappearance of all protection for successful skin selection. Prior work/negative probes remain preserved.

## Authorized Smart Color experiment — 2026-10-15

User «делай» authorizes the proposed independent Auto Balance + semantic Skin Protection + temporal stability prototype before AE integration. Scope/criteria SC01..07: [SMART_COLOR.md](SMART_COLOR.md). This extends color work; restoration, training, GPU acceleration and publication remain deferred. Current plugin Natural/looks/state and installed b7f44 bytes are preserved. The first backend's semantic quality failed on independent scenes, so integration is gated on replacing/improving that backend and repeating acceptance; running inference or a passing numeric kernel is not sufficient. Current exact results are recorded in STATUS.md. Earlier MVP obligations below remain open.

Current scope: user's color-first MVP steering, recorded in MVP_COLOR.md, takes precedence over the original v3 sequence below. V3 remains the broader post-MVP contract unless superseded by a direct decision. Initial GitHub repository was empty; preserved baseline b21b571. Branch dev/ai-enhance-prototype. No main/release/publication authorized.

## Discovery and delta
For AE artists processing phone footage, compressed SDR, low light, moderate softness, photographs and graphics. Apply effect -> automatic moderate Enhance/Fast/Neutral -> select Look -> adjust strength. No analyze button or model selector. Preserve night/light character, faces and clean footage.
AI is now core; HDR, upscale, interpolation and generative face changes are excluded. Custom .cube moves after MVP. Six looks: Neutral, Natural Skin, Warm Film, Cool Modern, Soft Vintage, Night Mood. Five main controls: Enhance, Look, Look Amount, Quality, Mix. Advanced controls only when algorithms make them meaningful. No independent framewise auto color or previousFrame. Python/PyTorch research/export only. JSON state and embedded versioned LUTs. Fast 33–40 ms at 1080p is target, not claim. User selected this Mac as initial reference.
Stage 0 is covered by supplied contract. Model/license/quality/speed unknowns block production selection, not independent research. Senior consultation was expressly authorized, read-only, no implementation ownership.

## Accepted rules
AE Development Rules v11.1.0, commit 04b606845e0f73ab28b9807bb45682eae4d6ce34, tag object 67359cba5ff85401c90455ccde803e65dcc91a85. Complete checkout ../AE-Development-Rules-v11.1.0. Smart Entry, Product Discovery; Process §§1–13; Engineering §§14,16–21,24–25,32,34–35,41; Native §23; transfer §26. Critical risk, Development delivery. Research-methodology governs audit; production-engineering governs implementation/native review/testing. Reference Audit N/A: model research inputs are not whole-product parity targets. Release, updater, IPC, CEP, UXP, HDR out of this block.

## Architecture / first slice
C++17 core without AE headers. Typed borrowed CPU RGBA float includes dimensions, stride, accessible buffer length, transfer/primaries, alpha mode. Immutable settings, explicit FrameWindow, context resources/backend/cancel. First experimental model FFDNet Color Clip is a small denoising path; NAFNet-SIDD-width32 is a quality comparator, no production choice yet.
One ONNX Conv/ReLU trunk; exact C++ shuffle/pad/sigma prep; shared loaded session, one active inference, reusable context buffers. Temporal radius explicitly zero; other windows rejected until implemented.
Initial AI slice supports opaque SDR with explicit sRGB/linear-sRGB. Reject unsupported alpha/extended input, never successful silent clipping. Mix=0 bypass remains exact; LUT-only uses explicit residual policy. These research restrictions do not remove full v3 requirements. No fake auto color, defect estimator, independent Denoise/Detail or skin recognition.

## Acceptance / traceability
| ID | Requirement | Check / remaining obligation |
| --- | --- | --- |
| AI-01 | Exact weights/code provenance and commercial redistribution rights | Model audit; rights unresolved => no weights redistribution |
| AI-02 | Model improves defects without face/detail invention | Real pretrained inference + paired synthetic metrics; natural video review open |
| CORE-01 | Typed bounds/stride/color/memory; no AE headers | C++ compile, bounds/stride/NaN tests |
| CORE-02 | Frame -> model -> LUT -> same dimensions | Upstream/ONNX/C++ parity; odd-size/RGB shuffle |
| MIX-01 | Mix=0 exact, Enhance=0 allows looks | Extended/transparent bypass + backend call-count |
| COL-01 | Model normalization/gamma, no inferred Rec.709/global clamp | Explicit color/unsupported-input tests; AE working-space later |
| LUT-01 | Tetrahedral CPU and exterior policy | Independent six-simplex/identity/domain tests |
| STATE-01 | Versioned JSON, embedded versioned LUT, preserved old state | JSON foundation; native save/reopen/Undo later |
| TEMP-01 | Explicit bounded window, cut/boundary behavior, order independence | Radius-zero repeatability now; video/SDK neighbor checkout later |
| CACHE-01 | Look change doesn't rerun restoration, bounded invalidation | Later after content identity and AE cache audit |
| AE-01 | Native automatic Enhance/Neutral/Fast; 8/16/32, alpha, ROI, scale, cancel | Stage 2 NOT RUN until core measured |
| MFR-01 | Bounded loaded sessions/parallel inference | Shared CPU backend tests; real AE MFR later |
| PERF-01 | Cold/warm/total times, RAM/VRAM, 1080p/4K | CLI/research MPS; AE/4K later |
| VIS-01 | Clean/night/faces/textures/text/motion/compression/cuts | Authored synthetic chart now; representative video corpus open |
| UX-01 | Five controls, meaningful advanced group | Native stage 2/4 |
| PORT-01 | Shared source, actual Windows/macOS evidence | Mac C++ CLI now; Windows/AE NOT RUN |

## Sequence
1. Candidate/license/environment audit, source/checksum pins, comparative bench.
2. Minimal C++ pipeline and ONNX CPU reference, parity, timing, synthetic rendered QA.
3. NAFNet and short-video quality; CoreML EP placement/precision investigation; select only from evidence.
4. AE prototype: Enhance, Neutral + one artistic look, Look Amount, exact runtime identity and total latency.
5. Temporal/cuts, adaptive/stable color, six looks, Fast/Best, bounded caches, actual MFR.
6. Both platforms and measured optimization/fine-tuning. No release implied.

Core standard library + small MIT JSON/SHA headers; external ORT; external Adobe SDK later. Source/assets all rights reserved pending owner's license decision. No imported footage/LUTs.

## Authorized continuation block 2 — 2026-10-14

User requests maximum useful steps in one run and short human progress after each step; senior-model code consultation is explicitly authorized. Restore/preserve baseline2f6f3f2. Development branch retained; no release/main/unknown cleanup. Bounded work:

1. Real-footage technical corpus with source rights/attribution, explicit color conversion and disjoint scene train/validation/test splits; authored corruption and unmodified source distinguished.
2. Official FFDNet gray/luma and IRCNN color timed against prior RGB model; excluded candidates record actual license reason.
3. If measured speed gate fails, bounded existing-architecture residual student/distillation research (not a new large network), train recipe and exact teacher/source provenance; held-out scenes untouched until selection frozen. Unknown teacher rights keep all derived weights research-only.
4. Core improvement: conservative defect/confidence/noise gate, clean exact bypass, temporal-statistics-only explicit window and cut boundaries; no temporal pixel mixing. Tests must show benefit on degraded input, not just a no-op mask.
5. Optional bounded fast transfer implementation only after independent error bound vs reference; retain exact reference and extended-value fallback. Real CoreML C++ path, active cancellation/admission tests, source/state serialization and six original look definitions where independent.
6. AE research adapter/bundle and owned-host checks only when dependent model/core quality gates permit the stated prototype scope. Incomplete selection is not production approval. Actual AE/Windows/runtime gaps remain explicit.

After each block update STATUS or this record with measured scope, evidence and next decision. Benchmarks run serially, separate from compilation/training. Final source review and affected checks required before push to dev branch.

## User steering — quality before acceleration

2026-10-14: establish which processing actually works before making it faster. This overrides the execution order above. Park further student training, color/backend optimization and AE integration; preserve their existing changes and failed experiments. A fast near-identity student is not success.

Q-01: compare source/no processing, ordinary blur, RGB FFDNet, IRCNN and NAFNet on identical short motion clips and identical corruptions. Include original material, weak/strong added noise, extra H264 compression and modest blur. Freeze cases/seeds/settings before looking at results. Explicit sigma15 is a research setting, not automatic estimation.

Q-02: evaluate the official SIDD homepage noisy/reference pair separately as real smartphone noise; it is one illustrative published example, not independent held-out certification (NAFNet was trained on SIDD). Preserve original-size crops, range/color/detail errors and negative results. No extrapolation to all phone video.

Q-03: compare unclamped float outputs against references, gradient error and motion-relative error changes. Inspect stills and prepare motion previews; visual motion inspection is a separate gate. Natural ToS source is already compressed/graded, not noise-free GT. Metrics from authored corruptions establish only recovery of those corruptions. Clean original damage, residual color bias, no-op and blur counterexamples are explicit vetoes for universal default selection.

Q-04: document defect-specific usefulness and outstanding automatic/temporal/color/native requirements, then select the next quality experiment. Speed targets remain open; do not optimize a failed quality path.

## Current priority — color-first MVP

Direct user steering: «нам прежде всего надо решить вопрос с цветом плоских исходников, чтобы можно было работать как с логарифмом. остальные улучшения оставь на потом после мвп». Restoration/model selection, denoise, compression, detail, temporal restoration, training and acceleration are deferred until after MVP. Their code, evidence and open requirements remain preserved. Model latency does not gate the color-only MVP.

Clarification closed: both ordinary SDR grading and true camera Log processing; no SDR-to-Log export in MVP. User delegated initial profile choice. Choose explicit Sony S-Log3/S-Gamut3.Cine first, using the official Sony formulas/primaries; ARRI source research preserved without expanding current delivery. Implement deterministic host/source/output contract, complete color-only frame path and checks; then native adapter/owned-host validation. No inferred camera gamut, framewise auto-color or recovered RAW/scene-latitude claim. Details and remaining acceptance in MVP_COLOR.md.

## Authorized metadata addition — 2026-10-14

User requested automatic useful embedded camera metadata reading. Implement META-01..07 in METADATA.md as a scoped color-MVP addition: bounded independent Sony NRT MP4 reader, automatic read-only ECW recognition/recommendation, explicit existing render profile/encoding. Preserve old color checkpoints and all broader obligations. No render-time filesystem work, inferred gamut, silent Log normalization or private footage publication. Native UI12 is appended without changing diskIDs1..11. Actual source/host acceptance must bind the new candidate; old356 host evidence is a regression reference, not proof of new UI.


## Authorized Cinema & Palettes addition — 2026-10-15

User requests recognizable film-inspired looks and clear English names, Natural maximally natural, retired old Warm, Teal & Orange plus eight supplied pairs. See CINEMA_LOOKS.md for CIN-01..06 acceptance, source/reference limits and compatibility. Natural stays exact neutral value1; old Warm value2 is a nonselectable separator retaining legacy output. Six film interpretations and nine palettes append3..17. Look Amount defaults100%, slider0..100%, typed0..300%; active color core extrapolates in existing host-linear space. Original v3 six-look names are superseded for this library by this direct choice; all other retained obligations remain. No AI/restoration/GPU work, merge or release.


## Size-adaptive face protection — authorized2026-10-15

Implement prototype per ADAPTIVE_FACE_MASK.md:24/64/160source-pixel height bands, smooth transitions, explicit simple-mask permission, bounded temporal associations/gaps without previousFrame. Numeric contract implemented/tested; fresh detection and actual motion failures retained. Quality/occlusion/continuous review, blur confidence and native integration remain open. Metadata/non-Sony/non-camera plans remain unchanged.


## Blurred-face anchors — authorized2026-10-15

Implement ANCHOR_FACE_TRACKING.md independent of current detailed-face detection. Geometry, detail selection and current foreground veto separate. Bounded tracking/core and local experiments complete; general semantic occlusion/continuous graded-motion quality still open. Do not promote on the single restored55frame.
