# Scalar toning — native development integration, 2026-10-18

## Scope and criteria

SCALAR-AE-001: reproduce the approved scalar-strength trial in the SDK-independent C++ scene core, then route AE through that implementation. No reference-frame RGB/mask transfer. Stable strength is explicit per-person analysis data; current pixels, LUT and strength supply grading at render time.

SCALAR-AE-002: preserve original analysis masks/samples, and optionally add soft current-frame support + unit scalar. Version1 cache compatibility; schema2 validates checksums, exact payload sizes, finite unit values, budgets and immutable snapshot. No model execution/Python inside AE. Existing cache snapshot GUID changes when analysis changes; selected LUT/Look Amount/Smart switch are excluded from analysis identity.

SCALAR-AE-003: verify Off→On reuse, look/amount regrade, upstream miss fallback, alpha/ROI, explicit order-independent inputs. New analysis backend/UI for arbitrary user footage is outside this bounded bridge: it reads externally prepared tensors. Without cache environment the older Vision path remains active.

SCALAR-AE-004: isolated real AE render and image comparison, plus moving multi-person/occlusion review. Do not equate a build or scripted render completion with visual acceptance.

## Changes

- `SceneLook::apply_stable_people`: current-frame native full skin result per person, scalar strength through soft support; convex overlap ownership; original scene result retained outside support. Zero support returns the old result exactly. Off bypasses invalid/unavailable analysis. Luminance/alpha preserved and one gamut limiter scales the chromatic residual.
- AE samples support bilinearly in the same layer/ROI coordinate system as existing fields; no hidden previous-frame history. Current-only input checkout in prepared-cache mode. Immutable context references rebuild for each requested look/amount.
- Read-only cache schema2 adds derived `-pN.support` files and `toning_strength` values. New field bytes count towards existing64MiB entry/128MiB resident cache limits. Maximum4resident entries; caller-retained shared entries/transient buffers are not a total-process memory bound.
- `prepare_strength_cache.py` reconstructs exact current geometry/material veto and scalar schedule from bound research evidence; exporter publishes checksummed immutable payloads. It writes no models. Prepared source input identity and original source height must match exactly.
- Optional developer environment `FSTR_LUT_SCENE_TRACE=1` emits cache RAM/DISK/MISS keys to process stderr. No image data or default source-frame logging.
- Build identity now includes scene-cache, scene-color and individual-tone sources/headers.

## Verification so far

- PASS:12/12release checks,12/12ASan+UBSan checks (includes actual loaded module callbacks, schema1/2cache, invalid strength, switches, look changes, premultiplied alpha and ROI).
- PASS:36/36real research frames through the new native stable method agree with the previous scalar research result within5.960464477539063e-8 linear float.
- PASS:C++ reader validates36/36schema2multi-person records; export221726016bytes total, policy `5b588038d08ad341d501d1ff44e83da23e41512e41b56a4fe5209ec2ae72497e`.
- PASS:29Python contracts; scoped static scanner exit0 at `/tmp/fstr-native-scalar-host-20261018-audit.json`. Vendor SHA helper emits pre-existing signed-conversion warnings; linker notes duplicate core library. Static result is not host certification.

Artifacts: `../strength-native-equivalence-20261018/evidence.json`, `../strength-native-cache-20261018/native-read-verification.json`, `../scalar-ae-host-20261018/`.

## Host candidate / ownership

New visible effect `FSTR-LUT Scalar Test`, match `FSTR LUT Scalar Trial 20261018`, distinct `FSTR_LUT_Scalar_Test_20261018.plugin`. Existing three LUT bundles are preserved. Nonce-gated AEGP helper installed separately as `FSTRScalarOwnedRunner_20261018.plugin`; no external script executes without exact nonce and an empty project. `installation.json` binds file inventories. This is a development test bundle, no release/push/main change.

Host source PNGs are research frames replicated4x nearest-neighbor to1920x800 for a declared quarter-resolution transport calibration (480x200, original source-height800). This does not create new camera footage or establish full-resolution model quality. Exact analysis-key hits must be confirmed from process trace; successful all-ordinary fallback is not analysis acceptance.

First direct binary launch exited without running the script. LaunchServices started a separate PID34164 while user PID97733 remained open. Read-only screenshot confirmed unsaved C2111; no mutation was sent to it. User then explicitly reported saving/closing their AE. Host candidate now continues startup. Real host results are pending; append observed outcome below.

Startup diagnosis: PID34164 sample showed AE's plugin-loader error modal during `CleanupGeneralPluginScan`, before any owned script. Local bundle verification identified incomplete Mach-O/bundle signature (`code has no resources but signature indicates they must be present`). Both newly owned bundles were archived and ad-hoc signed as complete bundles; strict verification passed and a fresh signed inventory was captured. No global security setting or unrelated plugin changed. SIGTERM did not exit that startup modal; the verified owned PID was stopped with SIGKILL before a signed retry. No project/script had started in the failed process. This attempt remains preserved, not counted as a passing host run.

## Observed host result and defect correction

The first signed, unique-ID candidate entered the script but failed export. A no-effect control rendered; neutral effect failed before Smart. The owned four-way comparison (`../scalar-ae-host-20261018-matrix/report.txt`) isolated the regression in both8/32bpc: old Cache Test and new no-GUID-flag diagnostic rendered; current and no-topic diagnostic failed. Advanced grouping was therefore not the cause. Successful neutral PSDs match the source within1code level at32bpc export and exactly at8bpc.

Final candidate adds `PF_Cmd_QUERY_DYNAMIC_FLAGS` capability/handler mirroring static capabilities (following the SDK25.6SmartyPants pattern), and mixes a fixed base seed in inactive pre-render instead of omitting the GUID path. Active prepared analysis mixes the immutable snapshot GUID. Runtime/PiPL/tests now agree on flags2`0x08221401`. The combined protocol change passed the failing host case. This experiment does not isolate the contribution of query handling versus inactive seeding, nor assert a general SDK requirement absent from its documentation.

**Actual AE25.6x101 final32bpc host run PASS**, `../scalar-ae-host-20261018-v3/`:
- no-effect and neutral controls; Off→On→Off→On; second LUT and150%amount; changed upstream exposure On/Off;36On +36Off frames.
-77native-reference image comparisons and3switch/upstream pair checks pass. Actual maximum1code level at8-bit Photoshop export (predeclared maximum2). This export precision does not establish float-exact host pixels.
- Trace:36DISK,4RAM,1MISS. First On reads disk; On reactivation and look/strength changes reuse the same RAM key. Changed exposure misses; it does **not** trigger a new model backend in this read-only bridge.
- Smart is materially active: maximum22code-level On/Off difference,591002frame-pixels with a>1level change across36frames. No all-ordinary false pass.
- Exact installed signed candidate additionally passes191native callback assertions. Final12/12release and12/12ASan+UBSan suites pass after the SDK protocol fix.
- Source is declared nearest-neighbor replicated research footage, **layer Draft**, quarter render480x200, unmanaged sRGB, opaque alpha. Real32bpc rendering was compared after8-bit PSD conversion. Host HDR/transparent footage/Best-quality scaling/MFR-specific invocation counts/old-project migration remain NOT RUN.

**Actual fresh-process restart and hard-cut fallback PASS**, `../scalar-ae-host-20261018-restart/verification.json`:
- Opens only the hash-bound owned saved project; first requested analysis is read from disk in a fresh AE process.
- Known prepared footage →unseen solid shot→known prepared footage. Unseen middle frame records MISS and matches Smart Off within1export code level. Known frames match native references within1level. No previous-mask carry into the unseen shot.
- This demonstrates safe unknown-input fallback, **not automatic shot detection or analysis of the new shot**.

Optional AEGP error handle is returned empty on successful jobs; the runner prints a blank `script error:` label for that handle. Host PASS reports + decoded output comparisons establish outcome; zero ExecuteScript status alone is not acceptance. The earlier nonempty script error remains a preserved failure.

## Separate final review / open scope

Reviewed native math, explicit per-person ownership, finite/unit/parser budgets, version1fallback, immutable snapshot dependency, thread-safe cache access, exception boundary, cancellation/ROI/alpha and actual signed artifact identity. Corrected the measured export regression; reran its failing host control and complete affected callback suites.

A36frame/3person/partial-occlusion sequence rendered with the scalar method inside AE and agrees with the measured standalone candidate. Enlarged actual-host snapshots (including frame10) inspected; irregular source/occlusion geometry still exists. Continuous perceptual flicker and all small/blurred-face plan sizes are **not accepted**. Actual AE comparison video: `../scalar-ae-host-20261018-v3/review/AE-Smart-Faces.mp4` (12fpsresearch cadence, enlarged crops).

The bridge is **prepared-clip development integration**: Python/inference stays outside AE; no analysis backend/UI for arbitrary new media is added. New material defaults to ordinary LUT until a matching new analysis snapshot is prepared. Time-remapping/editing the analyzed temporal context requires a new external snapshot; current pixel keys alone cannot validate a changed temporal neighborhood. Do not present this as complete production MASK-CACHE-001 or full automatic Smart integration. Auto pixel encoding, diverse real skin/lighting/crowds and continuous visual acceptance remain open.

Final receipt: [SCALAR_NATIVE_EVIDENCE.json](SCALAR_NATIVE_EVIDENCE.json) binds source files, exact signed installed effect, host reports/trace and comparisons. Helper archived after its final PASS with exact inventory verification (`../scalar-ae-host-20261018-restart/cleanup.json`); two diagnostic effect variants were archived earlier. The final isolated Scalar Test and all three original effects remain; no user project replaced. Snapshot frames10and35 inspected at960x1050review delivery size; this remains static inspection, not continuous perceptual approval. No release/push/main change.
