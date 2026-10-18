# SMART-COREML-VALIDATION-001 — full pipeline and owned AE trial

## Decision

Retain installed CPU Current **1c490d4515b2937b**. Experimental Core ML candidate **8ef20895ffc59d55** preserves the exercised output and reduces initialized uncached render time, but its synchronous initialization is not suitable for promotion. No product source, controls, IDs, defaults or installed Current bytes changed. No installer, release, push or merge.

Base source: a0553c8a9397aabf1521526605e121ffa100e841. Frozen overlays, drivers, source bindings, logs and results live outside Git at `../smart-coreml-validation-20261018/` relative to the repository. That directory is part of the durable local checkpoint, not a distributable package. Adobe SDK, models and footage remain external.

## Fixed criteria and results

Acceptance was fixed in local `contract.md` before execution. Mac M1 Pro16GB/macOS26.6.2 build25G83; AE25.6x101; ORT1.23.2; original scene model checksum be6fb001052144f89aa34e6d5dbea61265b484277e68e6a874073561907770d0. CoreML MLProgram/ALL/static shapes. CPU and CoreML have different analysis policies. Host compiler cache binds verified model SHA, actual loaded runtime SHA, OS build and fixed options. Host runtime SHA7745dad2f02e2956791ca84a1fb226bc73bd88a53ecd8a2b2471da536c60a6d4 differs from the original standalone runtime bytes; both exact bindings are recorded.

| Criterion | Inspected result |
| --- | --- |
| V1 full observations | PASS168frames, three shots/cuts72and120, six original prepared hashes exact. No exact accepted raw-cache match, so all CPU references freshly recomputed. |
| V2 masks | PASS same counts/boxes/support/descriptors/sample data; max role difference1.6331673e-5, person mask1.4901161e-7 against1e-4 gates. |
| V1 repeats | PASS six out-of-order repeats per backend, exact in exercised fields. Not a universal determinism guarantee. |
| V2 explicit windows/color | PASS174bounded same-shot windows;432comparisons across None+17original cube assets and strengths0/.65/1/3 on six frames, plus168TealOrange65%sequence frames. Max encoded RGB difference5.51343e-7 /2.98023e-7 respectively, against2e-5; alpha unchanged. |
| V3 cancellation/lifetime | PASS standalone immediate/timed callback cancellation then successful reuse; serialized session and RAII reviewed. Constructor/in-flight Apple prediction interruption and arbitrary native backend failure recovery NOT RUN. |
| V4 AE | PASS28exports and18comparisons within1byte PSD exportcode; source32/16/8bpc paths, cuts, SmartOff/On, LUT edits. Final PSD is byte output; this does not certify float host parity over arbitrary HDR values. |
| Cache behavior | PASS cold old/new windows each5inferences/no disk; warm old/new each5rawRAM+5preparedRAM/no inference. SmartOff leaves counters unchanged; re-enable/LUT edit do not run inference. Disk reuse observed after rawRAM eviction. |
| Visual sampled output | Inspected two native CPU/CoreML frame pairs at960x540, close kitchen and wide performers; no observed visual difference. Full new-backend continuous human viewing NOT RUN. |
| Ownership/preservation | PASS nonce/empty-project guard, saved own project, normal owned AE exit, trial/helper archived, installed Current exact inventory and strict signature verified. |

The168-frame input is the established edited DPReview NikonZ6Flat fixture with original watermark and existing shot recipes. It is not an untouched camera original or proof of recovered Log latitude. Standalone quality batches ran concurrently and their durations are not speed evidence.

## Actual AE timing

Each cold request has an explicit five-frame window, no cached masks, initialized model contexts and prewarmed compiler cache. Source960x540; masks384x216. Three interleaved CPU/CoreML pairs:

| Frame | CPU ms | CoreML ms |
| --- | ---: | ---: |
|35|19146|17948|
|95|16035|8645|
|143|11953|5953|
|Median|16035|8645|

Median elapsed reduction46.09%, ratio1.855. First CoreML request is much slower than later ones; no steady-state/long-video/1080p/playback-FPS or other-machine claim. Warm mask-reuse medians750msCPU/706msCoreML do not run the model and do not establish model acceleration. Native8bpc18275→7081ms and16bpc17770→8932ms are single pairs, not separate statistical benchmarks.

Loading remains a material limitation: standalone cold initialization68626ms, fresh-process warm17424ms; AE effect creation57827ms despite prewarm. CPU project opening5869ms is a different operation and is not a comparable initialization benchmark. Compiler-cache partitions retained their prewarm timestamps; an ORT namespace miss is not supported by these observations. Exact Apple internal cause remains unresolved. Actual AE profile contains86CoreML-provider events, proving provider execution; diagnostic compute-plan GPU preference does not prove per-operation physical GPU/ANE allocation.

## Failures and review

Preserved harness failures: first package completion looked in the wrong build directory; first probe lacked a runtime rpath and aborted before main; a comparison-board helper initially omitted frame suffixes. Corrected drivers and reran affected checks. They did not mutate Current or establish product defects.

Senior read-only review identified a300s error-path wait risk in ORT1.23.2 diagnostic ProfileComputePlan. It is enabled only in the earlier standalone diagnostic overlay and OFF in the host candidate. Review also rejected the proposed process-wide partition-ID cache-miss explanation; do not change keys based on that hypothesis. Session/input/output lifetimes and serialized inference are retained. Final review checked V1–V4 against actual reports; loading/cancellation/general footage limitations remain visible rather than being replaced by build success.

## Resume contract

Next implementation slice: managed background session initialization outside AE suite calls; publish an immutable ready analyzer, retain it through all in-flight requests and join workers before unload. Fix backend choice for a render request; do not switch neighboring frames when initialization happens to finish. Show an explicit loading/failure state; never report failed analysis as successful restoration. Validate AE cancellation, unload/reload, failure and worker lifetime before any default promotion. Preserve CPU Current and accepted42ee quality baseline. Do not promise that prewarming alone fixes loading.

Local evidence: `contract.md`, `quality-summary.json`, `mask-comparison.json`, `repeat-comparison.json`, `matrix-color/result.json`, `sequence-color/result.json`, `prewarm-verification.json`, `host-verification.json`, `host/host-report.txt`, `host/installation.json`, `host/cleanup.json`, `ae-package-inventory.json`, `ae-environment-binding.json`, source-binding files and ORT profiles. `ae-comparison.png` contains the sampled native frame pairs. `evidence-manifest.json` binds their exact bytes.
