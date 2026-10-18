# SMART-STAGE-PROFILE-001 — accepted-quality stage measurements

Date2026-10-18. User accepted shown quality («качество принимаю») and authorized continuing. This step measures before selecting acceleration. Accepted installed build **42ee1edaaff01753** and source implementation from6f96e81 are retained. Diagnostic source starts at repo0e653125088dfa5cd84383debb3ca613b5a4ffb6, on dev/ai-enhance-prototype. No product math/backend/cache behavior change, installer or release.

## Method and acceptance

External workspace: `../../smart-stage-profile-20261018/`. Its README definesP1..P6 and scripts; checks/results stay tied to exact diagnostic files, not to installed production performance. Diagnostic native/host scopes use steady-clock wall time, unique scope/parent/thread IDs. Whole model foreign calls, class decoding/refinement, mask construction, cache stages, AE callbacks and whole pixel loops are timed; no per-pixel tracing. Parent-inclusive and child durations overlap: use exclusive durations within one call tree, never sum parallel calls as CPU usage. Stdio tracing overhead is included in parents; cross-binary speed comparisons are not controlled overhead measurements.

Real inputs: frames0/35/72/95/120/143 of the already accepted three-shot sequence,960x540/24fps, known cuts72/120. AutoPrepared with the accepted per-shot recipes, neutral manual settings, unmanaged sRGB. Analysis is the same center-nearest384x216 opaque byte input as AE. M1Pro16GiB, macOS26.6.2, SDK25.6; model/runtime/assets unchanged. ONNX scene path uses the existing CPU session configuration, not a newly enabled GPU backend.

## Standalone measurements — PASS scoped

Six separately initialized diagnostic probes versus original probes: all raw scene roles, person masks, support and samples, plus person metadata, match exactly. Each cold request computes once; RAM then disk replay performs no new inference. Timers preserve the inspected numerical result.

Across those six raw analyses, median total2363.117ms; scene model Run median1776.431ms, range1636.290..2152.443ms. Run alone is76.22% of summed analysis time.150-class decode median124.174ms; edge refinement24.289ms. These are per-frame analysis measurements, not the complete AE frame path.

One initialized model context, two serial six-frame passes: second-pass median2302.791ms; initialization4322.723ms. Real cache paths on frame0: cold2604.980ms, RAM2.382ms, disk38.956ms. During a genuine different-key miss, an already-resident hit took2114.139ms. The existing resident mutex encloses inference and disk work; its blocking mechanism and actual warm-hit delay are confirmed independently of host timing. Current concurrency is retained for this experiment.

Three960x540 core primitive trials, five measured repetitions after one warmup: preparation medians83.329/83.034/92.104ms; ordinary LUT color medians107.019/108.584/114.320ms. Preparation and color outputs match the original CLIs exactly. These separate core primitives exclude the combined Smart pixel path, AE checkouts, input hashing and temporal work.

Evidence: `comparison.json`, `stage-summary.json`, `stage-events.json`, `batch-result.json`, `batch-stderr.log`, `standalone-summary.json`, `color/` in the external workspace. Fresh absolute paths/caches avoid reading accepted-analysis entries under a changed diagnostic policy.

## Host timing and cleanup

Distinct match `FSTR LUT Profile 20261018`, buildb52ac39d77a6e4b5, isolated preparation/analysis caches; accepted Current remains installed. First owned AE82877 completed13PSD exports and normal script-scheduled exit. Twelve unchanged-setting exports match accepted OFF/ON frames within1PSD8 export code; the remaining strength50 export is a changed setting. All diagnostic scope trees have valid same-thread parent containment and nonnegative exclusive times. Saved own Stage-Profiling.aep.

Six cold Smart frame requests take8.109..14.771s each, including3or5 new neighboring analyses; total24 raw inferences. Individual Smart color pixel loops take723.884..1163.973ms at960x540. Pre-render, bounded image preparation, full upstream-channel hashing, temporal stabilization, cache and color are separately logged. A strength65→50 edit performs5disk reads, no inference; color987.231ms, source-identity hashing303.700ms, disk reads211.739ms in an1848ms whole render. The exact timer totals are retained in host-verification.json.

The RAM cache has a hard4-entry ceiling, while a middle-of-shot Smart request uses5raw frames. The five-frame strength-edit request reads all5from disk; the dedicated three-frame boundary request establishes the actual RAM route. This behavior is independent of the resident-mutex blocking mechanism.

OFF→ON toggle and an IMAGE_CACHES purge reuse AE's already-rendered output (one pre-render event, no SmartRender/pixel loop). These153/163ms exports prove cached-output reuse, **not** an actual warm Smart pixel-processing rate. A further distinct-strength boundary test requires a real color_pixels scope and3RAM hits; it passes at strength55. Source project in the first trial briefly loaded its original effect before replacement, so both identities were loaded; second trial uses the saved diagnostic-only project. Actual production playback/FPS remains NOT_RUN.

Second own AE83861 rendered3morePSD exports, saved Warm-Profile.aep and exited normally. First request reads3disk entries with zero inference. Changing strength50→55 then executes the real Smart pixel path with3RAM hits, no disk and no inference:1509ms whole render,973.296ms color loop,199.858ms upstream hashing,68.207ms bounded image preparation,7.172ms total raw-cache calls,48.088ms pre-render callbacks. Returning to65again reuses AE output (152ms, no pixel scope); it matches accepted frame0within1export code. Only strength55 qualifies as the actual RAM-warm timing. No claim that all3second-trial requests execute the pixel path.

All16exports complete;13unchanged-setting comparisons against accepted exports pass <=1code (12first trial +1cached second-trial). Both signed diagnostic helpers and profile effect were inventory-checked and archived after their owned AE processes ended; the complete accepted Current inventory/signature remains unchanged. Evidence: host-verification.json, host-stage-events.json, warm-verification.json, host/cleanup.json. This closesP1..P6 for this scoped profiling task; it does not qualify a speed optimization or new product release.

## Selected follow-up

1. **Completed by [SMART-CACHE-LATENCY-001](SMART_CACHE_LATENCY.md):** separate short resident RAM/LRU locking from the serialized miss path. Keep inference serialized until concurrency is independently qualified. Recheck RAM after the miss gate; preserve cancellation/failure wakeup and clear-memory generation semantics. This removes a measured avoidable wait; it does not make the scene model faster.
2. **Completed by [SMART-COLOR-REUSE-001](SMART_COLOR_REUSE.md):** measure and reduce repeated per-pixel color work against the accepted outputs; model acceleration alone cannot establish a fast complete path.
3. Evaluate acceleration of the existing scene model on this Mac, comparing output and complete-path times. No downsampling/model replacement/FP16 or changed mask quality is accepted implicitly.

Historical failures: diagnostic color harness initially linked only fstr_core and missed render_sdr_preparation; corrected to fstr_sdr_prepare and rebuilt. First batch launch rejected a relative resources path before model load; corrected to absolute path. Neither establishes a product defect. Original logs/checkpoints remain local.
