# Cold and repeat rendering — implementation, 2026-10-20

Current state: **installed9bc2f053c92ddca5**, source1169bf3, scoped host acceptance complete. The pre-host records below retain their original checkpoint; the final acceptance section supersedes their pending statuses.

User authorizes acceleration of the first and repeated render without harming accepted quality. Base dd5f8e9, accepted installed7e28c1ba365f2131. No installer, release or publication. Preserve signed bundles, user projects, original assets and all unsuccessful evidence.

## Findings and rejected approaches

Current six-shot-sample diagnostic uses six real384x216 prepared images and the shipping CoreML/MediaPipe assets. Scene inference, per-face local inference and CPU postprocessing are real cold costs; historical CPU-only stage shares are not current CoreML shares. Initial device exploration overlaps builds and is not a qualified speed comparison.

A tiled150-class CPU decoder preserves all float masks in18frozen cases and invalid/cancel tests but gives no consistent gain in the clean ten-pair captured-logit benchmark. Removed from product; source and logs retained in ../cold-performance-20261020/rejected-tiled. Its28Release/28ASan+UBSan results do not verify a shipped change.

GPU-only CoreML versus current ALL: a separate same-process run retains one context per option and alternates48complete raw analyses over six inputs, with no analysis cache. After6warmup pairs,18measured pairs have medians928.107ms for ALL and947.572ms for GPU-only.80corresponding payload files and person metadata agree exactly. No demonstrated gain, so no production backend change. Neural-Engine-only exploration is slower and has small mask differences; it is also rejected. These are native-analysis experiments, not full AE render timings. Test caches/tensor capture stay outside the plugin; no frames/tensors are written by the product.

## Zero-ownership colour candidate

`SkinToneLook::people_from_ordinary` supplies a zero correction mask only when that person's ownership is exactly zero. `apply_from_ordinary` retains unit/confidence/reference/binding validation before returning ordinary colour for that zero mask. Convex accumulation stays in the original order. Every nonzero soft edge retains the full original correction. No change to LUT, precision, models, masks, temporal behavior, public types, controls or source interpretation; no allocation, private worker or new shared state.

Frozen skin/scene fixtures are namespace-isolated originals from5f2ae8c, verified byte-equivalent to dd5f8e9 after undoing namespace/include isolation.693120 current/frozen bit-output and exact exception comparisons include16people, zero/-zero/mixed ownership, invalid/stale references, nonfinite/extended pixels, alpha, stable support, four LUTs and strengths0/.65/1/2.5.28SDK-free Release/28ASan+UBSan checks pass. These do not establish AE performance.

Trial21155b53695da3ac is a separate effect, built with CoreMLON. Native analysis compile identity17913797ac0c209448e722b56cbda5c2f39765a943415a7b3ae369c98f681a6f matches installed7e, preserving compatible mask keys. Production model asset/helper sources are unchanged. The first launch guard found an open user AE and stopped before installing anything; user subsequently confirmed it closed.

## Acceptance and pending work

Require SDK/OCIO Release and ASan+UBSan checks, source review, and actual alternating1080p forced renders with loaded candidate identity, pixel comparisons,5RAM hits/zero inference per measured warm request, cancellation/supported bit-depth regression as applicable. Accept performance only from repeated complete AE timings, not a component count. Preserve installed7e unless the candidate qualifies. Full new cold AE clip timing, GPU colour implementation, cache-first model startup and split disk/inference scheduling remain open. This document is a working implementation record, not a completion or release claim.

## Bounded stage overlap — candidate, not installed

The AutomaticSceneCache API defaults to one compute and permits an explicit maximum of two for a concurrently safe callback. The native adapter now requests two in the unpromoted candidate. NativeSceneAnalyzer already protects each individual Parts/Scene/Face model with its own mutex; no mutex was removed or shortened, no model context added and no private pool created. Host caller threads can overlap different stages of two current-frame analyses. The scene mutex still encloses inference and decoding, keeping only one scene logits result live at a time in this context.

Replaced the whole-miss mutex with per-key/per-clear-generation single-flight. Separate bounded lanes allow2disk reads,1writer and1(default)/2(explicit)complete computes; compute leases include persistence to bound the writer backlog. Resident mutex is never held during callback, cancellation or disk I/O. Failed/cancelled owners wake waiters for independent retry; clear forbids old RAM publication. Existing root writer locks/atomic files/checksums/quotas stay intact. No mask key/schema change.

29Release/29ASan+UBSan checks and3targetedTSan cache checks pass, including existing cancellation/clear/failure/reentry tests and new blocked-two-computes/queued-third, independent disk read, single-flight and persistent concurrent-write cases. Foreign model/system libraries are not TSan-instrumented; their per-model serialization remains in place.

The added native_pipeline probe runs48real analyses of six real384x216 inputs over4alternating six-frame batches per limit, one shared shipping model context, every case using a fresh owned cache. After the first warmup pair, batch medians4920.413→3172.583ms,1.551x for this native-analysis workload. All4serial/pipeline result sets agree bit-for-bit in scene/person masks, full double skin samples and person metadata. This does not establish a whole AE render gain, memory bound across arbitrary host workloads, or playback FPS.

Actual full1080p cold/warm AE trials, bit-depth/toggle and real UI cancellation are pending. Diagnostic candidate uses separate baseline/candidate test caches, timing-only callback/model scopes and a private match name; production paths and installed7e are untouched. No installer/release/push.

Final local review before host handoff:33SDK/OCIO Release and33ASan+UBSan checks pass; after adding a stronger explicit two-generation/two-compute regression, all3affected cache targets pass again in Release, ASan+UBSan andTSan. No user callback runs while a resident/lane mutex is held; each flight/result lifetime, exception path, clear generation, cold-result bound and native model mutex was traced separately. No observed local defect remains in this reviewed scope. Static scanner reports no findings in its scope and does not assess release readiness.

Diagnostic candidate33ddeae846f092d5 and an exact embedded679complete-export full-sequence/depth/Off/recovery scenario are prepared. The full-host launch guard found a newly opened unsaved user project and stopped before copying either temporary module. User work was only inspected, never saved, closed or edited. Actual cold/warm full AE export, memory observation, real UI cancellation/recovery and canonical installation remain NOT_RUN. Current installed7e is unchanged. Resume from ../pipeline-performance-20261020/launch_host.py after the user closes AE; do not rerun completed experiments. The test helper saves only its own new project before waiting, checks ownership at every deferred entry/render, and leaves a different project open instead of closing it during cleanup.

## Final actual-host acceptance — 2026-10-20

Evidence root: ../pipeline-performance-20261020/. Implementation source1169bf3996fb3b3a14138962955cbf68910ad06f; private trial33ddeae846f092d5; canonical installed9bc2f053c92ddca5. The canonical snapshot differs from reviewed source only in the established internal registration name. Timing scopes and candidate-only cache prefixes are absent from the installed package. Source/inventory bindings, signatures, exact loaded-module records and the separate preinstall result review are retained.

### First versus repeated rendering and cache fairness

Same Mac M1Pro16GiB, macOS26.6.2, AE25.6x101,168frames/7seconds/1920x1080/24fps,32bpc, MFR requested100%. Both builds run the same source, controls, shipping models, analysis resolution and all168fresh observations. Separate initially empty analysis and SDR-preparation roots prevent the candidate from borrowing baseline results. Models are initialized before timing. These are complete export durations after initialization, not model startup or playback FPS.

| Actual full pass | Accepted7e | Pipeline trial | Less elapsed |
|---|---:|---:|---:|
| Baseline first, candidate second; separate empty plugin caches |254502ms|172409ms|32.25%|
| Candidate first, baseline second; new empty plugin caches and AE ALL_CACHES purge before each pass |230874ms|140514ms|39.14%|

First pair1.476x, reversed pair1.643x on this clip.168real analyses per build/pass are counted; candidate callback spans cover168distinct frames on8host threads, with8overlapping callbacks but only2complete analyzer calls at once. Per-model mutexes remain intact. OS file cache was not purged; compiled-model caches were retained. The reversed run addresses AE source-frame caching and order bias; this is not a cold-Mac or universal-footage benchmark. Model loading before the reversed pair took271.176seconds; other startup observations are much shorter. Startup is explicitly excluded and remains unfinished work.

Three full disk-warm pairs, all with0new model calls and168disk observations per pass: old70354/new74463ms; old92309/new80253ms; old68499/new53983ms. Orders are new→old,old→new,new→old. Both ranges overlap; separate medians70354/74463ms do not support a reliable repeat-render gain. Do not advertise a warm multiplier or choose only the favorable pairs. The demonstrated gain is fresh-analysis throughput.

### Correctness, host lifecycle and failed harness attempts

845full old/new1080p image comparisons pass within1PSD8bit export code:339initial matrix,337warm/cached-recovery,169reversed/fresh-recovery. Opaque alpha and output geometry checked.178initial and168reverse corresponding raw scene/person/support/sample/header cache entries are byte-exact; reversed entries also match the initial baseline. Native48analysis test separately compares full double samples, unlike the float32 persistence codec. Representative paired frames35/95/143 were visually inspected with no observed difference. New continuous human motion acceptance is NOT_RUN; earlier quality acceptance is retained, supported here by exact raw masks and whole-sequence numeric comparisons, not relabeled as a new viewing session.

8/16bpc conversions correctly generate five new observations per side in the isolated first matrix; SmartOff generates no analysis/cache accesses. Both full warm runs and regrading reuse stored masks. No plugin errors occur in completed full passes.

The first cancellation attempt fails in the harness before rendering: Natural Contrast is a checkbox, so1.04 is out of range. Original output preserved. Followup0.96 is accepted as true and does not invalidate analysis: it establishes only cached-render USER_STOPPED/recovery. The final reverse test correctly changes the Contrast slider to1.04; two active native analyses are observed immediately before the CUA Escape action. Real host USER_STOPPED and expected517aborts follow; resetting Contrast permits old/new recovery renders with0new analysis and matching pixels. No failing harness attempt is counted as fresh-cancellation PASS. Report decoding also accommodates AE's non-UTF8 error quotes without rewriting original bytes. Original invocation errors/notes are retained separately.

All three diagnostic sessions exit normally. Temporary effect/helper modules are archived only after exit, with exact inventory checks; installed7e stays unchanged throughout these trials. A one-second startup sample shows both loaders in CoreML/Espresso on-device compilation; it is taken before timing and is not evidence of a render-stage lock failure.

### Canonical installation and final review

Separate actual-result review verifies132reviewed source inputs, frozen code parity, bounded synchronization/cancellation,845export comparisons and both cold orders;11ignored Python bytecode files are excluded from source compilation. Canonical SDK failure/Off recovery and compiler-lock/generation checks pass. Rejected tiled decoder and alternate compute-unit experiments remain excluded.

With all AE sessions closed, install9bc2f053c92ddca5 and preserve signed7e exactly at ../pipeline-performance-20261020/current/previous-installed/. Canonical owned AE75043 loads that exact module/inventory and renders15outputs with15prior comparisons within1export code. Default product caches supply48stored observations with0new model calls. Cover three host depths,SmartOff/re-enable,LUT edit and saved Custom slot18 open/save/reopen. Both saved projects are owned copies. Normal exit and helper archival pass; installed package and rollback signatures/inventories remain exact. See current/verification.json,current/host/loaded-module.json,current/host/cleanup.json and acceptance.json in the evidence root.

Final scope: this acceleration slice is accepted and installed. GPU colour, cache-first/lazy inference startup, broad hardware/footage performance and playback FPS remain NOT_RUN/unimplemented. No installer, release, push or merge. Neither the local scanner nor this performance record certifies whole-product readiness.
