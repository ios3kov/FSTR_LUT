# Active checkpoint — 2026-10-14

Explicit resume completed the metadata/camera trial block. Source8733709 and BuildID32597669ec8fb8e1 are saved; STATUS.md/EVIDENCE_COLOR_MVP.json carry current checks and limits. Owned PID40072/birth19:15:50 saved its unique project and closed normally; fresh process inspection confirms no regular AE/aerender remains. Exact temporary helper moved to ../installed-archive/FSTRColorOwnedRunner-metadata-v1-completed.plugin and is absent from MediaCore. Current color bundle remains installed for user trial, old356 retained in archive. No reset/deletion/main merge/release.

The previous pause records below are historical, not active stop instructions.

---

# Resumed on explicit user instruction — 2026-10-14

Development is active. The checkpoints below are historical. Fresh process inspection confirms no regular AE/aerender process before the new owned metadata/camera test. Preserve all previous source, artifacts and evidence. The current final block is actual metadata UI and camera-color validation, applicable color regression, evidence reconciliation and a limited trial package; no main merge or release.

---

# Paused for the third time at the user's explicit request — 2026-10-14

No more work/installation/AE/UI/commits/push until the user explicitly resumes. Existing changes, builds and negative evidence remain saved; nothing reset/deleted. The user's reply to the AE-close question was a PAUSE, not confirmation that the current user AE is closed or may be controlled.

HEAD remains0768901 on dev/ai-enhance-prototype. Metadata source/tests/spec changes are UNCOMMITTED. Installed color effect remains356ecd25a276063e; the new metadata candidate has NOT been installed. Current separate SDK helper remains the prior color-host-final-v3 exact instance; no helper replacement occurred this turn.

New implementation now builds: signed Release metadata/color bundle BuildID32597669ec8fb8e1 in build-ae-metadata-v1/FSTR_LUT_Development.plugin. All5 final exact-candidate suites PASS in ../color-evidence/metadata-candidate-final.log; strict local codesign verification PASS. Earlier final Release5suites PASS also in metadata-final-release-v2.log. ASan/UBSan core/metadata/native adapter and new owned metadata UI5suites PASS in ../color-evidence/metadata-sanitize-final.log; actual AE is not instrumented. Sanitizer run preceded the final SDK-header-binding-only CMake adjustment; tested executable source behavior is unchanged by that identity adjustment. Do not call these actual host GUI acceptance.

Closed prior defects: struct DrawObjects semicolon; malformed-file first ftyp classification; scanner now walks structured moov/udta/meta/xml full-box boundaries and parses complete XML documents (no substring promotion); multiple roots and conflicting boxes reject; effective child namespaces enforced; valid prefixed NRT accepted; bounded depth8/4096boxes/read4MiB/XML256KiB. Added tests for double-root/wrapper/raw-moov/prefix/depth/foreign namespace. Pure C++ CLI confirmed local supplied clip Sony ILME-FX3 / Rec.709 gamma and primaries with bytes_read2100: ../C2098-metadata-core.json. Serial/GPS remain excluded. Private footage/full XML/frames must not be pushed.

New tests/metadata_ui_tests.cpp invokes the actual dlopened EffectMain with owned SDK/Drawbot stand-ins and real parser: automatic caption, cache redraw, click refresh, missing/proxy/composition/source error, malformed UTF16size/NUL/surrogate and cleanup failure fallback, unrelated-event no-op and teardown. Native render fixture still checks only params1..11/world suite; new custom NO_DATA UI12 is excluded from rendering. Senior review found lone-surrogate conversion path; explicit surrogate validation and regression case added. Source/render encoding remain manual. No camera metadata is copied into render sequence/settings. Source lookup suite failures now show manual fallback. Actual UI drawing remains NOT RUN.

Accepted metadata delta/observable requirements are now in docs/METADATA.md (META01..07) and appended PLAN.md. tinyxml2v11.0.0 pinned9148bdf719e997d1f474be6bcc7943881046dba1/zlib license preserved. Source BuildID now includes new metadata/Drawbot SDK headers. No new model/acceleration work.

Prepared, NOT EXECUTED: ../color-host-metadata-v1/config.json, nonce869fa19b-ea28-4f80-8797-e3e0e70e5ce5; protected own camera script generated from tools/ae_host_runner/camera.jsx.in. Signed separate helper built at that folder/helper-build. Intended owned AEP only; C2098 direct-source effect UI neutral, plus resampled precomp Original/Grade/Warm three PSD frames at clip time2s; explicit Gamma2.4/Host settings, declared2/255 reference tolerance. Source movie dimensions/rotation must be inspected in actual outputs. Script keeps only its saved owned project open for UI inspection; do not rerun after an unknown effect.

Latest read-only process snapshot before pause: regular user AE PID30666, birth2026-10-14 18:08:14, no -m in command. Not owned/inspected, do NOT close/save/reuse or screenshot its project. CUA path lookup selects the oldest regular AE instance, hence asked user to close current AE after saving to enable owned metadata UI test. User paused instead. Verify process state on resume; do not infer closure. Independent progress remains possible if user session prevents UI.

Next upon explicit resume: restore exact candidate/spec/source state; safely archive old owned356 only after exact hash check and install one new candidate; replace exact owned temporary helper with new job; launch nonce-owned AE -m. Validate actual registry/source metadata control with C2098, redraw/refresh/source replacement/fallback without touching foreign projects; compare Original/Grade/Warm to independent core and visually inspect actual user footage locally. Repeat appropriate color-state/MFR regression on new exactcandidate (old356 evidence is historical reference). Then reconcile STATUS/EVIDENCE/NATIVE_REVIEW (currently stale7fbe), record final source/bytes/checks/gaps, commit/push dev branch, package outputs guide/currentbundle, archive temporary helper after verification. Real SLog clip/Windows/direct floatworld/Undo/RAM-preview/hostROI remain open. No main merge/release.

---

## Previous checkpoint — preserved

# Resumed again on explicit user instruction — 2026-10-14

This pause record is historical; development is active. See STATUS.md and METADATA.md for current acceptance.

# Paused again at the explicit user request — 2026-10-14

Do not continue implementation/builds/AE tests/installation/commits/push until explicit resume. Everything below is preserved on disk; no reset or removal.

HEAD0768901 (development branch dev/ai-enhance-prototype) commits the corrected native package and owned acceptance tools. Current installed/signed color bundle remains356ecd25a276063e, byte inventory verified against build-ae-packaging-v2. It was NOT replaced by the unfinished metadata build. The source of core color math remains25819a9. Documentation STATUS/EVIDENCE_COLOR_MVP still needs reconciliation: its old7fbe/locked-Mac status is superseded by actual host results.

Verified actual AE25.6 before this second pause:24 SDR/syntheticSony output frames in8/16/32 projects; corrected fresh-process MFR range0..3 (4frames);5 outputs from alpha/WB save-reopen/linear-working-space/WarmFilm script, plus one separate fresh aerender alpha output. Numeric reports PASS against predeclared2/255 export tolerance; alpha differs by <=1LSB from8bit dither. Alpha references multiply unclipped straight float by alpha BEFORE integer export clipping; an initial analysis mistakenly clipped first and was corrected without changing tolerance. Sources/reports: ../color-host-sdk-render-v4/{numeric-report,mfr-numeric-report}.json; ../color-host-final-v3/{host-report.txt,numeric-report.json}; ../color-host-alpha-aerender/aerender.log. 32bpc expression float baseline remained uncalibrated0,0,0,0 and is NOT PASS. No direct negative/HDR float-world acceptance, Windows, realSonyLog clip, manualUndo/RAMPreview/hostROI/downsample certification.

User provided local C2098.MP4 and does not remember its profile. Local embedded NRT XML extraction (Python only, not the new C++ scanner) confirms Sony ILME-FX3, CaptureGammaEquation rec709, CaptureColorPrimaries rec709, CodingEquations rec709. Private XML may also contain serial/GPS: keep ../C2098-nrt-private.xml LOCAL ONLY; never push the clip, its frames, full XML or identifying source path. Whitelist report ../C2098-profile.json; original ../C2098-metadata.json; locally inspected decoded preview ../C2098-preview.png. The clip and existing projects are unchanged. Its SDR comparison through the plugin is still pending.

NEW explicit authorized requirement: automatic useful camera metadata reading if present. Scoped decision communicated: automatically read/display declared camera/gamma/primaries when Effect Controls custom area draws; do not silently change Source/AE Pixel Encoding because SDK cannot establish all input/prior CM conversions. Read outside render; show manual recommendation. No background scan without visible ECW was promised. Need record this delta/observable criteria in METADATA.md and PLAN/MVP_COLOR before further dependent design.

Uncommitted metadata prototype: include/fstr/metadata.hpp; src/metadata/metadata.cpp; tools/metadata/main.cpp; tests/metadata_tests.cpp; src/ae/metadata_ui.cpp/.hpp; CMake/plugin/PiPL/nativefixture edits; vendored tinyxml2 v11.0.0 commit9148bdf719e997d1f474be6bcc7943881046dba1, zlib license/provenance preserved. Bounded MP4 top-level seek/read4MiB/XML256KiB/4096boxes, skipsmdat/free; whitelist manufacturer/model/gamma/primaries/coding only. SDK PFInterface source discovery only in documented EVENT::DRAW. Source path/UTF16handle suites/Drawbot are scoped; UI cache128 entries by effect/path/mtime/size, renderer never accesses it. New NO_DATA custom UI persistentID12 appended; renderer continues checkout1..11. HistoGrid exact SDK sample establishes NO_DATA+PF_PUI_CONTROL+custom UI registration. New runtime flags0x02008440 (CUSTOM_UI added), flags2unchanged. Click clears UI cache, then DRAW refreshes. Metadata UI is NOT runtime tested.

Current build FAIL: ../color-evidence/metadata-native-tests-v2.log, missing semicolon after struct DrawObjects in src/ae/metadata_ui.cpp:116. Stopped without fixing because user requested pause. Earlier parser suite failed assertion25 for unknown-file classification; source fix moved ftyp check before length validation, but no successful rerun yet. CLI C++ read of C2098 was NOT RUN because build chain failed. Core-only build log ../color-evidence/metadata-core-build.log; native initial log metadata-native-build.log. No successful metadata sanitizer/runtime claim.

Senior read-only review found mandatory unresolved parser issues: (1) MP4 scanner extracts first NRT root and ignores remaining payload, accepting conflicting roots within same box or an NRT nested in unrelated wrapper; must fail closed for multiple candidates and unproven document boundary, with real scanner tests. (2) child local_name ignores namespace rebinding: foreign Device/Group/Item can be promoted; validate effective NRT namespace before recognizing fields. (3) parser supports prefixed root but scanner only unprefixed; align contracts (explicit supported subset or implementation). DrawObjects RAII now covers exception cleanup, but semicolon missing. Byte limit is not wall-time limit on slow volumes; synchronous UI read limitation remains. Reviewer did NOT execute tests or AE.

Temporary owned SDK helper remains separately installed as MediaCore/FSTRColorOwnedRunner.plugin; exact ownership ../color-host-final-v3/helper-install.json. It is inactive in unrelated processes without nonce. Last v3 SDK job completed guarded empty-project quit; user reports AE closed. Do not assume current process state on resume: verify PID/birth/executable/nonce and project ownership before UI/closure. Old screenshot request was auto-review rejected because stale foreign-instance context; process ownership proof showed onlyownedPID21156/oldforeignabsent. Re-review did not reject, but CUA timed out on startup modal; sample showed BirthPrefs dialog. That job later completed. Never change/delete user preferences to bypass startup and never reuse foreign projects.

On resume: restore current requirements and local changes; fix/cover the parser gaps and syntax defect; run meaningful metadata/core/native/sanitizer checks; test actual custom ECW on owned C2098 source and source replacement/missing/proxy fallback; prove render settings/colors unchanged and no metadata I/O in render; finish local C2098 comparison. Preserve356 artifact/evidence before replacing it; only one active FSTR matchName. Then reconcile docs/evidence to exact currentsource/binary, commit/push development branch and create outputs guide/package. Keep all wider color-MVP checks open when unperformed. No main merge/release/publication authorized.

---

## First pause and resumption — retained history

# Resumed on explicit user instruction — 2026-10-14

This pause record is retained as history. Work is active again; current results belong in STATUS.md.

# Paused at the user's explicit request — 2026-10-14

No further implementation, builds, host tests, installation, commits or publication are authorized while paused. Resume only on user instruction. Working changes remain on disk; nothing reset or removed.

Branch dev/ai-enhance-prototype; HEAD390063d482b42fb29c202cdee425a0dd3b74866c. Current uncommitted changes: native package identity eFKT/FXTC/PkgInfo/resource filename; guarded render harness fixes; new tools/ae_host_runner standalone SDK helper and acceptance script. Core/native image processing source remains25819a9e379a49caaa94df4fa5e2af5112b5cd07.

Current signed/installed color build356ecd25a276063e is at user MediaCore/FSTR_LUT_Development_356ecd25a276063e.plugin. Wrong-packaged7fbe bundle/output preserved, superseded; do not deliver it as the accepted host candidate. STATUS/EVIDENCE_COLOR_MVP.json/NATIVE_REVIEW.md still describe the older checkpoint and need reconciliation after resume.

Confirmed before pause: actual AE25.6 native registration and24 exported PSD8 frames in projects8/16/32 match standalone SDR/Sony references within2/255; outputs are8bit and cannot establish HDR. New aerender process reopens own32bpc project, MFR ON50, corrected range0..3 gives4/4 PASS(max0.0039062135359819727). No measured concurrent callback peak. Original range1..4 includes a transparent out-of-composition frame; negative/range evidence retained. Reports: ../color-host-sdk-render-v4/numeric-report.json and mfr-numeric-report.json. EXR fixture generation roundtrip is exact, planesG/B/R/A=[2,.5,-.25,1]. Alpha/WB/Warm references and tolerances were declared before host output inspection: ../color-host-final/reference-manifest.json.

Already launched owned combined test process remains running at pause: exec session34794, nonceb21be05f-fa28-4c1d-beec-9e6d3632e1ff, own folder../color-host-final. Latest report contains only HOST/FLOAT_TRANSPORT/FLOAT_BASELINE0,0,0,0; this is INCOMPLETE and not a passing extended-value probe. Do not assume completed or rerun after an unknown effect. Read its actual report/log/process state on resume, diagnose the test baseline before assigning a plugin defect. Helper is installed separately as MediaCore/FSTRColorOwnedRunner.plugin, exact files in ../color-host-final/helper-install.json; compiled nonce guard makes it inactive in every unrelated process. Old exact helper archived at ../installed-archive/FSTRColorOwnedRunner-render-v4.plugin. Archive the temporary helper after hash/ownership verification when its host work ends; it is not part of the shipped plugin.

Foreign user AE instance/project must remain untouched. CUA getApp binds the oldest foreign instance, not the owned SDK job. Earlier owned processes may remain open; verify PID/birth/executable/nonce and actual project ownership before closure. Do not force-terminate uninspected projects.

On explicit resume: inspect existing combined job; finish/critically review alpha/save-reopen/extended-expression probes using actual outputs; preserve failed attempts. No new product features were promised for this block. Then reconcile current evidence/docs, commit/push development branch, package current356 bundle under outputs with a clear limited development guide. Real camera Log footage, Windows, manual UI/Undo/RAM-preview and host ROI/downsample remain unverified. Do not call full MVP/release ready or merge main.
