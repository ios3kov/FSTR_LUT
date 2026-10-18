Current working build is nowed9e33d687019dc1, including this unchanged cache improvement plus [exact color reuse](SMART_COLOR_REUSE.md). The7be installed/runtime measurements below remain tied to their original artifact;7be is now archived in the color-reuse task.

# SMART-CACHE-LATENCY-001 — resident cache latency

Scope authorized by continuation after SMART-STAGE-PROFILE-001. Base6e4a1bd, branchdev/ai-enhance-prototype. Preserve accepted42ee1edaaff01753 as quality reference. No color/model math, input encoding, controls, cache keys/policy/version or disk-budget change. No installer/release.

## Requirements and implementation

C1. A resident hit must complete while an unrelated real analysis is running. RAM/LRU has a short dedicated mutex; disk reads, validation, inference, writes and eviction use a separate serialized miss gate. Never hold resident while waiting for miss or invoking cancellation/backend/I/O. Double-check RAM after acquiring miss, so equal misses still coalesce.

C2. Keep shared backend calls serialized. This change does not qualify NativeSceneAnalyzer for concurrent inference. One cache object must outlive its calls, as before.

C3. Cancellation is per request; RAII releases the gate/reentry marker after owner cancellation, errors and allocation failures. A waiting caller can cancel without blocking resident access or cancelling another owner. A recursive miss of this same cache on its owner thread throws logic_error instead of waiting on itself; recursive resident reads remain valid.

C4. clear_memory clears RAM immediately and increments its generation. Requests begun before clear can return/persist valid results but cannot republish them into the new RAM generation. This is RAM clearing, not cancellation/disk invalidation. Retired entries release outside the resident lock; returned shared_ptrs remain valid.

C5. Increase the entry ceiling4→8 so one explicit5-frame temporal window fits, with some nearby-frame room. The existing128MiB resident byte cap remains authoritative; no promise that every arbitrarily large window fits.512MiB/4096 disk budgets, entry validation, recovery and write ordering remain unchanged.

C6. Preserve accepted output and validate actual host behavior, not only builds/cache files. Evidence workspace: `../../smart-cache-latency-20261018/`.

## Verification

19Release tests and19ASan+UBSan tests pass; Debug sanitizer flags confirmed on the new test and modified library. Two targeted TSan cache tests pass with instrumented cache code. Existing tests cover disk recovery, process exclusion, stale/corrupt entries, key/source invalidation, same-key coalescing and budgets. New behavior tests use held-owner promises to prove resident independence, immediate clear/generation, cancellation callback reentry, failing/cancelled owner retry, serialized different misses, recursive misses/resident reads and five-frame pointer reuse. No assertion relies on a model being fast.

Separate read-only senior review found no concrete defect; it did not execute tests or establish AE/MFR acceptance. Actual execution evidence is separate.

Real native-model probe on the same prepared source images: uncontended RAM2.310ms; resident hit while another real key still computes2.287ms, versus the previous measured2114.139ms. The owner is confirmed still running when the resident hit returns. Native model policy remains exactly562c790e54360c35da1cd3dba1ec5c753c2932ce8b88681fd8bd7d74ff48d5df, equal to the original accepted native probe; model initialization/cold inference are not claimed faster. This isolates removed wait, not whole-frame speedup.

Trial eae8accd337cc93c, distinct match FSTR LUT Cache Latency20261018: own AE87248 completes12exports,9accepted-baseline comparisons <=1PSD8code, and15RAM hits across three changed LUT/strength requests. No disk/inference on those edits or Smart toggles. Model requests stay sequential. Trial/helper archived after normal own-process exit; accepted Current remained unchanged throughout that trial.

Canonical working build **7be976a9a5160b0e** is now installed under the existing Current filename and match **FSTR LUT Auto Current20261018**, so saved projects retain their effect identity. Complete signed42ee1edaaff01753 bundle is preserved byte-exact at `../../smart-cache-latency-20261018/current/previous-installed/FSTR_LUT_AutoSmart_Current_20261018.plugin`. It remains the quality reference. The new Current keeps all existing controls/IDs/defaults.

The exact installed7be build then passes another owned AE89393 session:12exports/9accepted-baseline comparisons <=1code. Its isolated cache was seeded by copying the valid trial cache; all24raw analyses load from disk with **zero model inference**, demonstrating compatible persistence across process/build identities. Edits of strength65→50, LUT9→2, strength50→55 produce5RAM hits each, zero disk reads and zero inference. These changed-setting renders take1593/2631/1648ms; they are scoped observed renders, not a controlled overall speedup/FPS claim. Returning Smart ON can reuse AE's output cache, distinguished from an actual processing pass by the new-value RAM counters.

Both own projects were saved. Both own AE sessions exited normally. Trial/helper installations were inventory-checked and archived; canonical test helper is archived. New installed Current inventory/signature and archived accepted baseline inventory/signature are verified. Complete installation, golden comparisons, counters and cleanup receipts remain local. There is no installer or published release.

Separate final review requires product source/asset fidelity, unchanged model policy, correct resident/miss lock order, targeted cancellation/generation/concurrency tests, actual canonical AE comparisons and signed bundle ownership. Source/manifest receipts bind the inspected artifacts. The late additional queued-clear regression case was rerun in Release,ASan+UBSan andTSan; product source/binary is unchanged by that test-only addition.

## Next work and limits

C1..C6 pass for the inspected scope. The cache improvement removes unrelated-miss blocking and five-frame disk thrashing; it **does not** accelerate per-pixel color, source hashing or scene-model inference. Next step is eliminating redundant color work against the retained quality reference. Production playback FPS, broad resolutions/modes and parallel-model operation remain NOT_RUN. A passing code scanner is scoped static evidence; release readiness is not assessed.
