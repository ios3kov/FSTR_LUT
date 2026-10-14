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
