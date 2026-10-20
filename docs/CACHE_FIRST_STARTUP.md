# Cache-first Smart startup — SMART-CACHE-FIRST-001

Base89c0cfe; installed9bc2f053c92ddca5 remains protected. User authorizes finishing safe acceleration. No installer/release/push. Implement startup before a separate GPU colour experiment; do not change model, analysis resolution, precision, colour or cache schema.

Requirements and acceptance:

- CF1: verify all model/runtime assets, manifest contracts, binary digests, backend/OS and the unchanged native algorithm fingerprint before accepting persisted masks. Derive exactly the current policy without creating model sessions. Missing/tampered assets fail Smart explicitly even with cached masks; Off remains ordinary grading.
- CF2: publish the existing bounded AutomaticSceneCache after verification. A complete disk/RAM hit must not construct an analyzer. Actual AE must render compatible saved masks with0model-session starts, not only0inference calls.
- CF3: on a true miss initialize one retained analyzer using an owned background worker; preserve two bounded compute lanes and existing per-model mutexes. Compare full constructor policy with verified policy before publishing new observations. Concurrent misses share initialization; cancellation belongs to each waiter; failed initialization is explicit.
- CF4: teardown joins initialization before code unload; no host API from workers, no abandoned threads, no implicit previous-frame state. Preserve cache validation, invalidation, schema and quota behavior.
- CF5: independently compare the reader policy with the accepted actual analyzer (persisted host headers carry a source-bound derived policy); frozen unit/host pixel oracles must pass. Test cache-only startup, cold miss, asset mutation, failure, cancelled wait, concurrent callers and shutdown. Release/ASan+UBSan/targetedTSan and actual AE cache/miss/Stop/recovery are separate gates.

Design: add a verification-only identity reader, leaving all fingerprinted observation algorithms/full constructors untouched. This preserves compatible installed masks. The reader checks bytes but cannot create masks. Full constructors remain the ABI/tensor authority on a miss and must reproduce the same policy. This avoids falsely freezing a changed algorithm hash or introducing a cache migration. Verification-only code is included in package source identity; existing native observation fingerprint continues to cover every algorithm/constructor source.

Cache state and model state are distinct: verified cache can be ready while the inference resource is idle. New DeferredResource reuses BackgroundResource lifecycle; first actual compute starts its worker. Factory owns all captured paths/policy, receives only shutdown cancellation, and never calls the host. Resources are held through shared cache/compute lifetimes. UI reflects loading/failure when the model is requested.

## Verification checkpoint — 2026-10-20

Implementation is not promoted. Accepted installed9bc2f053c92ddca5 remains unchanged.

| Gate | Result / exact scope |
| --- | --- |
| Core Release | PASS30/30 |
| SDK mock Release / ASan+UBSan | PASS34/34 each; vendor/host not instrumented |
| Targeted TSan | PASS5 lifecycle/cache/pipeline checks |
| Actual native policy | PASS exact accepted analyzer policy; verification-only probe about399ms, no model sessions |
| Asset/manifest mutations | PASS16 native and16 sanitized cases; originals restored/exact |
| First actual cached AE render | PASS1080p,5disk reads,0model starts; pixels within1export code of accepted reference |
| First cancellation attempt | FAIL status3015 after Escape; no cancellation/recovery acceptance inferred |
| Controlled Stop/retry | BLOCKED: owned AE87211 at startup modal before private modules; Mac locked / window reader unavailable |
| Canonical promotion / installed lifecycle | NOT_RUN |

Separate final source review covered deferred ownership, wait cancellation, exception retention, one retained worker, teardown joining, unchanged observation fingerprint, policy comparison before publishing new results, and host callbacks staying on caller threads. The pre-cancel guard was corrected and2affected targets passed again in core/SDK/ASan builds. Remaining host lifecycle gates block promotion.

Evidence outside Git: `work/cache-first-startup-20261020/` with check logs, mutation reports, `first-cached-pixel-check.json`, original failed host report and controlled retry. Retry8be24512bee8528d contains a test-only model-loading gate; it is not canonical product source. Its helper and private plugin remain installed while the owned AE process lives; cleanup requires its verified exit. Do not remove loaded modules or touch user projects.

GPU research is separate in `work/gpu-colour-20261020/`:30latest component cases,11strict numerical failures, alpha exact. It is neither integrated nor installed; no GPU user-speed claim. Preserve all failures and the accepted installed rollback.

