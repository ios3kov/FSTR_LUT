# Cold and repeat rendering — implementation, 2026-10-20

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
