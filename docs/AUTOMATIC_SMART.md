# Automatic Smart integration — 2026-10-18

Baseline: b2a1e03, dev/ai-enhance-prototype; macOS arm64 / AE25.6 / SDK25.6. Existing working tree clean. User accepts absence of visible flicker in the focused actual-AE fragment (frames12–29, central face, normal research cadence12fps and half speed). This is acceptance of that fragment only; no general temporal-quality claim.

## Current color requirement update

On2026-10-18the user canceled Auto encoding entirely and selected explicit sRGB default with manual Advanced override. SRGB-DEFAULT-001 in COLOR_AUTO_FOLLOWUP.md supersedes the Auto requirement in the historical sequence below; Auto is not an outstanding blocker. Smart analysis/visual obligations are unchanged.

## User acceptance update — 2026-10-18

User reports that Smart checks passed on the installed new Current build d774807231370f48. This supersedes the pending visual acceptance for that user trial in the historical notes below. Preserve the accepted local-median/current-geometry algorithm. Four-frame anchors/SAM2 parity remain separate experimental quality work, not a reason to rerun the accepted trial. Interrupted-cache-write recovery now passes on new Current build450636dfa3af11bb (see CACHE_RECOVERY.md); prior Off/On pixels match exactly. Broader unperformed host workflows retain their evidence limits.

## Authorized scope and acceptance

AUTO-SMART-01: unseen upstream pixels must run native analysis and use current scene/person evidence, without prepared clip files or Python in AE. Do not silently substitute the older face-only effect for the accepted combined algorithm. Native model ports must be compared to their research counterparts before promotion; changed analysis geometry requires new visual acceptance.

AUTO-SMART-02: bounded independent analysis cache; Off never invokes analysis; On/Off/On and look/amount changes reuse analysis. Current pixels, explicit neighboring context, source scale, input interpretation and model/algorithm identity invalidate relevant entries. No previousFrame, shared mutable per-frame state or cross-cut borrowing. Errors cannot masquerade as successful analysis.

AUTO-SMART-03: native ownership/ABI, serialized bounded inference, cancellation, thread-safe immutable data, validated schemas/models and source-safe persistence. Model loading outside render callbacks. Preserve all installed originals and project state; install only an isolated trial after standalone checks.

AUTO-SMART-04: compare real new-clip native/AE results; exercise switches, upstream change, restart, cut, save/reopen. Encoding follows SRGB-DEFAULT-001: explicit sRGB default with Advanced overrides; Auto was canceled.

## Implementation sequence

1. Prove the native model boundary (existing six-class MediaPipe and scene model) with independent reference output and resource lifetime/error checks.
2. Port current people/scene fusion, bounded temporal context and scalar-strength aggregation; record differences from accepted prepared trial.
3. Integrate automatic cache population and AE callback dependencies, then run isolated real host tests.
4. Use the verified explicit sRGB default and complete ordinary user workflow verification.

Python remains conversion/reference tooling only. External models/runtimes stay outside Git. Existing prepared snapshot remains a compatible development path. No training/restoration/GPU performance expansion, release, push or main changes.

## Implemented / current evidence

- Native C++ model boundary for UperNet scene, six-class MediaPipe materials, YuNet faces, MediaPipe478point mesh and four-person pose masks. All contexts created during GlobalSetup, before frame callbacks. Bundled runtime/assets in the isolated candidate; no Python/model server or externally prepared clip analysis is needed by that candidate.
- SDK-independent native fusion, individual source samples, continuous source-pixel detail bands, current mesh/body-material veto, two-sided local scalar median and conservative identity matching. This is a qualified development port, not full numerical/visual parity with the prepared SAM2/four-frame-anchor path. [Exact differences and model evidence](NATIVE_MODEL_PORT.md).
- Automatic raw-frame RAM/disk cache: four resident entries/128MiB resident payload budget,64MiB per entry; up to4096 complete disk entries/512MiB complete payload budget. Caller-retained frames, model arenas, transient writes and crash-left temporary directories are outside those byte limits. Version1 temporary-directory recovery is implemented; see CACHE_RECOVERY.md. Atomic directory publication, schema3 checksums/provenance and strict source/processing identities; original schema1/2 prepared cache retained.
- Smart Off bypasses cache/inference. On/Off/On, look and amount reuse raw analysis. Original pixel channels (including sub-8bit changes), pre-grade interpretation/settings and source scale participate in identity; look/amount/mix do not. Temporal grading is derived afresh from explicit input checkouts; no stale final temporal-mask cache or previousFrame. Build ID and model/runtime policy participate in the AE GUID.
- Core/cache/window and loaded-SDK regressions:14/14 Release checks pass. Loaded packaged real-model callback trial passes61643assertions (mostly pixel-alpha checks); Off/On exact repeat, actual activity, look/amount reuse and whole-code-level + sub-8bit upstream recomputation. Separate native-core comparison max1.7881393e-7 display RGB,2e-6 bound. These are not AE loader/runtime evidence.
- Two independent fresh-process current-frame model/fusion runs with disjoint empty caches produce byte-identical roles, all3person masks/support/samples and exact metadata on the group control. This closes repeatability only for that native CPU case, not the old MPS/SAM2 path or all footage.
- Models/runtime bytes, prepared inputs and full receipts remain external at `../native-smart-20261018/`. First isolated AE attempt at `../automatic-ae-host-20261018/` stopped before rendering: AddProperty could not find the requested match name. Attempt preserved. Shorter unique identity + explicit enumeration/registration trace at `../automatic-ae-host-20261018-v2/` succeeds. Cause is not established merely by shortening the name (SDK match-name capacity is48bytes, larger than the original34byte name).
- Actual AE25.6 trial B:34 PSD exports after32bpc rendering, Smart On/Off/On, look/amount changes, a real upstream one-frame cut,12 motion frames and save/reopen succeed. Off repeat, On repeat and cut On-versus-Off differ by at most one8bit export code. Separate process at `../automatic-ae-restart-color-probe-20261018/` opens the hash-bound owned saved project, renders frame17 using5DISK /0COMPUTE, and matches the prior render within one code. Its later diagnostic AddProperty failure does not erase that restart result. These host checks apply to trial B bytes, not every later root build.
- Independent real-model SDK callback replay with explicit five-frame windows is compared to all12actual AE motion frames, both On and Off (24image checks). Maximum error1.00098codes in the8bit PSD exports, below the predeclared2code bound; no pixel exceeds that bound. This checks native-versus-host pixel transport on trial B, separately from the earlier core color oracle. Exact data: `../native-smart-20261018/host-reference/motion/verification.json`.
- The root-only final correction fades detailed face toning support across64–80source pixels instead of switching it on at64. Live-model boundary test at63.99964/64.00036pixels yields support mass0/0.000002197. Root Release and ASan/UBSan14-target suites pass after the fix; trial B predates this one-line correction. Separate corrected trial Current at `../automatic-ae-current-20261018/` loads in AE25.6 and passes four Off/On/Off/On host exports: oneCOMPUTE, oneRAM hit; current On frame0 exactly equals preceding trial B. Its explicit five-frame native frame17 On/Off outputs exactly equal trial B float outputs. This qualifies the affected switch/load slice, not a repeat of the full motion/restart suite or a host mesh-threshold boundary test.
- Fixed-crop, unamplified actual-AE comparison video: `../automatic-ae-host-20261018-v2/visual-review/automatic-smart-comparison.mp4`, frames17–28,12fps, three repeats. All video frames decode. First still inspected at960×540: labels/crop readable; this does not close continuous temporal perception or approve the native port generally.
- AE-AUTO-SWATCH-001 diagnostic now registers and renders16working-space/instance combinations in AE. All six float swatches alias exactly across unmanaged/sRGB/linearized/Rec709/AdobeRGB/P3. Render callback observations report worker execution. This method is experimentally rejected as an Auto resolver; product encoding behavior is unchanged. Details: [Auto follow-up](COLOR_AUTO_FOLLOWUP.md).

## Still open

Current corrected bundle now has scoped loader/switch evidence; full motion/restart/save-reopen replay on later maintenance bytes beyond scoped tests; experimental fixed four-frame anchors and full-shot/SAM2 parity, tiny/blurred/crowd/diverse-tone coverage, time-remap/preview/MFR/Undo acceptance beyond the scoped trials, cancellation inside uninstrumented inference. Version1 temporary-cache crash recovery is now implemented under CACHE-RECOVERY-001; legacy unversioned leftovers remain untouched. Auto pixel encoding is canceled by the user; explicit sRGB default and Advanced manual choice replace AE-AUTO-001. Historical swatch/freshness negatives are preserved, but no Auto bridge is required. Do not call the full authorized block or production MVP complete.

## Checkpoint review / handoff

Separate review against AUTO-SMART-01..04 covered raw-pixel/processing identity, immutable publication, concurrent miss serialization, host-boundary exception handling, GlobalSetup/Setdown model ownership, C result/image/string release, explicit cut/window dependency handling and schema1/2compatibility. The confirmed64px support activation defect is fixed and regression-tested. Remaining model accuracy, full-shot parity, whole-process memory limits are not closed by these checks. Version1 temporary recovery now has separate scoped evidence in CACHE_RECOVERY.md.

Final source/host evidence index: [AUTOMATIC_SMART_EVIDENCE.json](AUTOMATIC_SMART_EVIDENCE.json). Developer code is behind defaultOFF `FSTR_NATIVE_SMART`; current installed trial is **FSTR-LUT Auto Smart Current**, match `FSTR LUT Auto Current 20261018`. Original installed effects are preserved. All owned diagnostic helpers have been archived after hash verification and process exit. No release, push or merge. SRGB-DEFAULT-001 now passes actual host migration/save/reopen and output checks on build d774807231370f48; see SRGB_DEFAULT_EVIDENCE.json. User-trial Smart acceptance is closed; broader workflow qualification is limited to its performed checks. No Auto development is required.

Whitespace check: authored source/docs/tests pass `git diff --cached --check` with the vendored NOTICE excluded. The complete installed MediaPipe LICENSE/NOTICE are retained byte-for-byte (both verified against the1.1.1wheel); upstream NOTICE contains trailing whitespace, intentionally preserved rather than rewriting legal source material.

Current trial update: CACHE-RECOVERY-001 build450636dfa3af11bb supersedes the installed sRGB-default build for maintenance only; prior accepted bytes are preserved. Six host exports and interrupted-write cleanup pass, with exact accepted Off/On pixel equality. See CACHE_RECOVERY_EVIDENCE.json.
