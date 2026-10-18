# CACHE-RECOVERY-001 — interrupted native analysis writes

Baseline d79c3e4, development branch. User reports that Smart checks on the installed sRGB-default Current trial passed; retain that acceptance for those installed bytes. Do not replace its accepted temporal algorithm with the experimental four-frame anchor schedule in this change.

## Contract

1. On cache initialization, and before a new disk publication, remove abandoned **version1 managed temporary directories** inside the configured automatic analysis cache. Never remove complete entries as part of recovery; existing FIFO entry/byte limits remain separate.
2. All version1 writers and recovery share a nonblocking OS advisory lock for that cache root. A live writer excludes recovery, including in another AE process. Process termination releases the lock. The lock file stays in place so contenders cannot lock different replacement inodes. A busy/unavailable lock skips maintenance and disk writes; it must not discard valid analysis or wait for the other process.
3. Only exact `.building-v1-UINT32-UINT32` real directories qualify. Preserve foreign paths, top-level symlinks, complete entries, and legacy `.building-*` writes: legacy writers do not participate in the new lock protocol, so their liveness cannot be established safely. No blanket temp-folder deletion or age-based guess.
4. Disk schema, raw analysis identity, model output, Smart Off/On reuse and color behavior remain unchanged. Unsupported platforms retain prior persistence without recovery. This slice targets macOS; Linux uses the same POSIX route but is not qualified here. Cache root filesystem requires local advisory-lock and atomic-rename support.

## Acceptance

Actual-process interrupted writer is reaped before next cache initialization removes its partial directory. An actively locked directory survives initialization and publication attempts; valid in-memory analysis still succeeds. Cleanup must preserve unrelated files/directories, complete cache entries and top-level symlinks; never follow links nested inside an abandoned directory. Existing reuse, corruption repair, cancellation and FIFO tests must remain passing. Release and ASan/UBSan checks are separate from actual AE loading.

## Implemented and verified — 2026-10-18

`RootWriteLock` uses nonblocking `flock` on a stable regular-file descriptor opened with CLOEXEC/NOFOLLOW/NONBLOCK. Lock scope covers cleanup, invalid-entry removal, temporary publication and FIFO eviction. Maintenance at construction is best effort. No schema/analysis/window/color formulas changed. Review covered descriptor lifetime, lock contention, process-exit release, name range/format, symlink behavior, immutable published data, exception fallback and unchanged budgets. The root must be the owned local cache; cooperating writers must keep the lock inode intact.

PASS: regression fails before implementation with “Abandoned managed cache write survived startup”; full14Release and14ASan/UBSan tests pass after implementation. The cache test uses a real child process holding the lock, confirms maintenance/publication skip while active, terminates the child without destructors/unlock, reaps it and confirms recovery. It also exercises legacy/foreign/name-overflow preservation, outward links inside the orphan, a top-level managed-name link, an unsafe lock symlink, complete-entry restart reuse and cleanup before publication. Windows/Linux qualification NOT_RUN.

PASS actual AE25.6: new Current build450636dfa3af11bb runs six32bpc/quarter-resolution PSD exports. A seeded managed partial is removed on initialization; legacy and foreign fixtures survive; one complete cache entry remains. Smart Off/On/Off/On, look and strength changes produce1COMPUTE/3RAM/0DISK. Prior Current-frame Off/On decoded pixels match exactly; repeat exports differ by at most one8bit code. This is cleanup/cache/color compatibility evidence, not a newly repeated full-motion visual acceptance.

Installed owned Current trial updated after exact old inventory verification; accepted sRGB build preserved at `../cache-recovery-20261018/accepted-srgb-current.plugin`. Fresh source matches root except trial registration names. Installed signature/inventory and preserved prior inventory pass; the test helper is archived only after its AE process exits. No other installed effect was changed. No release/push/merge.

Evidence: [CACHE_RECOVERY_EVIDENCE.json](CACHE_RECOVERY_EVIDENCE.json), external `../cache-recovery-20261018/{release.log,sanitize.log,host-verification.json,host/}`. The host builder, installer, nonce script, exact package and inventory are retained beside those logs. Legacy unversioned crash leftovers are deliberately not auto-deleted; unlike new writes, their ownership/liveness cannot be established by this protocol.
