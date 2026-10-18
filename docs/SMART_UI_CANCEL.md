# SMART-UI-CANCEL-001 — actual UI cancellation and recovery

## Scope and result

Base ad7c04d54c2b7964097f44a21b6e07da3bc0f5b6. No product source changes. Exact signed experimental candidate **5c0eac31093f15b0**, AE25.6x101 on the same M1Pro/macOS26.6.2. **PASS actual native GUI Escape cancellation while Loading, followed by successful same-process retry.** This closes the previously unrun GUI cancellation path for this one render-queue scenario. It is not coverage of every Stop control, playback or MFR cancellation.

Working installed CPU Current **1c490d4515b2937b** remains exact and signed. No installer/default promotion/release/push. Nonce helpers and trial bundle are archived after normal exit; own project saved as UI-Cancellation.aep. Existing projects/preferences are preserved.

## Procedure and observed criteria

U1: owned empty-project guard opens only the existing test fixture and duplicates its comp. A fresh private compiler cache ensures actual model loading. Render Queue starts a one-frame960x540/32bpc TealOrange61.314159265% Smart request. This exact amount has no prior5c0e host-output cache entry; the previous8ef synchronous output is the independent persisted reference.

CUA binds the window with the exact own-project title and sends **Escape**, requested epoch1792361240455ms and returned1792361242006ms. Host reports USER_STOPPED3017 at1792361242153ms rather than DONE3019; real pre-render abort517 is expected. The1698ms request-to-host-return includes CUA/input delivery and is not a pure keyboard latency benchmark. A subsequent actual screenshot visibly shows Render Queue **User Stopped** in the own project. Screenshot was inspected in the CUA conversation output; local ui-action.json records the observation, not fabricated pixels.

Before and after first attempt: begin1/ready0/model0/disk0/RAM0. No complete first-attempt PSD exists. This is cancellation of a model-loading wait, not inference, a finished frame or scripted cancel API.

U2: retry is scheduled30s after the actual stopped return, still in the same process and initially still Loading. It succeeds after one READY publication and exactly5new raw model computations, no disk/RAM mask hit. Loading never restarts: total BEGIN1/READY1. The successful retry PSD differs from previous synchronous CoreML cold-new-35_00035.psd by at most **1byte export code**, with identical dimensions and opaque alpha. The decoded960x540 output was visually inspected; no observed new defect. No universal full-video artistic/float/HDR acceptance inferred.

U3: UI state after Escape is readable and screenshotable, normal owned shutdown and cleanup pass. Current inventory/signature are verified unchanged. Loading/failure text in the effect info panel and failure-dialog painting were not shown in this Render Queue scenario and remain **NOT_RUN**. Concurrent cold compiler writers also remain unverified; CoreML stays experimental.

## Failed first attempt, preserved separately

First run armed8s after setup at strength65. Window snapshot during loading timed out and reset the CUA kernel; re-binding later auto-launched a new empty AE after the own process exited. That empty launch was identified and closed without altering projects. No Escape was delivered before completion. First job reports FAIL no GUI cancellation; model0/ready1 confirms host reused the completed output cache after waiting. It is not cancellation evidence. Complete first report/cache/helper are retained under attempt1-capture-timeout/ and attempt1-coreml-cache/.

Second run uses20s arming, fresh compiler cache and the exact uncached61.314159265% amount, sends Escape immediately after START FIRST, and avoids waiting on a screenshot before cancelling. No product change was needed. Actual ADD in these cold trials was5302/3145ms, not the1261ms previous warm measurement; do not generalize one add time.

## Evidence and final review

External local root ../smart-ui-cancel-20261019/. contract.md fixed U1–U3; verification.json separately asserts USER_STOPPED vs DONE, expected abort only, zero cancelled work, one loader publication, exact retry count and byte parity. ui-action.json binds the CUA request/return observation. host/test.jsx/config.json/helper build and inventories bind nonce/job; host/cleanup.json confirms preservation. retry.png is the inspected decoded result. SMART_UI_CANCEL_EVIDENCE.json records hashes. Weights/SDK/footage stay external.

Separate final review: U1 and U2 PASS, U3 PASS for inspected Render Queue/responding-window/cleanup scope; info-panel/failure views NOT_RUN. No observed product defect; first failed harness attempt is retained rather than counted as success. No product tests were rerun for this unchanged candidate; prior22Release/22ASan+UBSan/TSan evidence retains its original scope.

Next concrete integration work: loading/failure info-panel inspection and explicit cross-process ownership for cold compiler-cache creation before any CoreML default promotion. No pending user action.
