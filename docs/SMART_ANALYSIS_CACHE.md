# Native analysis cache — 2026-10-17

MASK-CACHE-001 implementation slice: native Vision observations (boxes, detail and holes) are cached independently of LUT/strength/Smart state. This is not the new whole-scene/SAM2 native backend. Raster region weights and bounded-window fusion are still evaluated at render time.

## Contract and implementation

`RegionCache` hashes exact prepared analysis RGBA bytes, geometry, original source height and algorithm identity. Algorithm identity binds native analysis/cache source, JSON/SHA dependencies and macOS implementation version. Hashes do not contain look, strength, Smart switch or effect instance. Identical normalized inputs can safely share observations. Any changed hashed input/version misses; recomputing unaffected entries is unnecessary. No source pixels are persisted.

A process-local LRU retains64entries; disk retains64complete entries. Each serialized entry is limited to1MiB on read and0.5MiB payload on write; geometry/count/finite bounds are validated. These are entry limits, not a whole-video retention guarantee. Large timeline traversal evicts old analysis. Disk eviction is oldest-write, not LRU. Persistent location: user Caches/FSTR.Tools/FSTR_LUT/native-regions-v1. An absolute `FSTR_LUT_ANALYSIS_CACHE` override is used only by the isolated test. No UI setting added.

Checksum, schema and key mismatch are misses. Failed persistence still returns a successful newly computed observation; failed detection propagates and is not cached. Empty valid detections are cached. Concurrent threads in one process share a locked miss; other misses serialize as the existing Vision device already does. Separate AE processes may compute the same cold key concurrently. Exclusive temporary files, checked writes/close and atomic rename prevent publishing partial entries; RAII cleans temporary files on handled failures. Crash-orphan maintenance and strict cross-process byte quotas remain future work.

Smart Off and neutral bypass skip the cache lookup/detector entirely. Existing RAM/disk entries survive. The accepted explicit user request for disk caching supersedes the prior blanket no-render-filesystem-loading restriction for bounded analysis-cache access only; no model/network loading was added to render callbacks. Disk read latency/abort responsiveness still needs a production budget.

## Verification

Native unit contract checks callback invocation counts for reuse, memory clear, new instances, changed pixels/scale/model version, corrupted disk bytes, concurrent misses, failure retry, valid empty detection, unavailable directory and entry limit.10release and10ASan/UBSan targets pass on this Mac.

Actual AE25.6x101, arm64, isolated `FSTR LUT Cache Trial 20261017` / visible `FSTR-LUT Cache Test`:32bpc one-frame source, AE caches purged before every render. First Off produced no analysis file; On produced a record; Off→On, another look and strength150 retained its name/modification timestamp. An upstream Fill produced another content key. A second full AE process reused disk records without modification.13renders completed; cache snapshots are recorded in external host reports. Native unit callback counts and source routing support the interpretation of host file snapshots; host does not expose detector-call counters.

Test PSDs, saved project, source/build hashes and host reports remain external. Original installed plugin and earlier Smart Host Test were not replaced. The separate Cache Test remains installed; owned AEGP helpers were archived after process exit. User saved/closed their project themselves before the owned host tests.

## Remaining acceptance

NOT RUN: this cache with SAM2/whole-scene inference, arbitrary-resolution production memory budgets, actual MFR duplicate-call counters, cancellation while waiting for a cache lock/disk, cross-process eviction stress, crash recovery, Windows, full-video mask retention and old-project migration. Do not describe all five requested Smart completion items as complete because this cache passed.
