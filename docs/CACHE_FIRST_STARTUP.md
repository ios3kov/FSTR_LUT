# Cache-first Smart startup — SMART-CACHE-FIRST-001

Base89c0cfe; previous9bc2f053c92ddca5 protected as signed rollback. User authorizes finishing safe acceleration. No installer/release/push. Implement startup before a separate GPU colour experiment; do not change model, analysis resolution, precision, colour or cache schema.

Requirements and acceptance:

- CF1: verify all model/runtime assets, manifest contracts, binary digests, backend/OS and the unchanged native algorithm fingerprint before accepting persisted masks. Derive exactly the current policy without creating model sessions. Missing/tampered assets fail Smart explicitly even with cached masks; Off remains ordinary grading.
- CF2: publish the existing bounded AutomaticSceneCache after verification. A complete disk/RAM hit must not construct an analyzer. Actual AE must render compatible saved masks with0model-session starts, not only0inference calls.
- CF3: on a true miss initialize one retained analyzer using an owned background worker; preserve two bounded compute lanes and existing per-model mutexes. Compare full constructor policy with verified policy before publishing new observations. Concurrent misses share initialization; cancellation belongs to each waiter; failed initialization is explicit.
- CF4: teardown joins initialization before code unload; no host API from workers, no abandoned threads, no implicit previous-frame state. Preserve cache validation, invalidation, schema and quota behavior.
- CF5: independently compare the reader policy with the accepted actual analyzer (persisted host headers carry a source-bound derived policy); frozen unit/host pixel oracles must pass. Test cache-only startup, cold miss, asset mutation, failure, cancelled wait, concurrent callers and shutdown. Release/ASan+UBSan/targetedTSan and actual AE cache/miss/Stop/recovery are separate gates.

Design: add a verification-only identity reader, leaving all fingerprinted observation algorithms/full constructors untouched. This preserves compatible installed masks. The reader checks bytes but cannot create masks. Full constructors remain the ABI/tensor authority on a miss and must reproduce the same policy. This avoids falsely freezing a changed algorithm hash or introducing a cache migration. Verification-only code is included in package source identity; existing native observation fingerprint continues to cover every algorithm/constructor source.

Cache state and model state are distinct: verified cache can be ready while the inference resource is idle. New DeferredResource reuses BackgroundResource lifecycle; first actual compute starts its worker. Factory owns all captured paths/policy, receives only shutdown cancellation, and never calls the host. Resources are held through shared cache/compute lifetimes. UI reflects loading/failure when the model is requested.

## Verification checkpoint — 2026-10-20

Installed16e472812ee35e41 accepted. Implementation774e5b6; exact signed9bc2f053c92ddca5 rollback preserved. No installer/release/push.

| Gate | Result / exact scope |
| --- | --- |
| Core Release | PASS30/30 |
| SDK mock Release / ASan+UBSan | PASS34/34 each; vendor/host not instrumented |
| Targeted TSan | PASS5 lifecycle/cache/pipeline checks |
| Actual native policy | PASS exact accepted analyzer policy; verification-only probe about399ms, no model sessions |
| Asset/manifest mutations | PASS16 native and16 sanitized cases; originals restored/exact |
| First actual cached AE render | PASS1080p,5disk reads,0model starts; pixels within1export code of accepted reference |
| First cancellation attempt | FAIL status3015 after Escape; no cancellation/recovery acceptance inferred |
| Controlled GUI cancellation/retry | PASS scoped keyboard Escape: abort517, QUEUED3015/renderingfalse, zero stopped output, deferred idle; cached render with modelReady0 then fresh retry5analyses using exactly1session |
| Matching accepted reference | PASS7/7 same-settings1080p outputs <=1export code, including changed-contrast cold-retry; accepted9bc reads same saved masks with0newanalysis |
| Canonical installed lifecycle | PASS15/15matching prior outputs, maximum0export code difference;48disk masks/0analysis/0model sessions,8/16/32,SmartOff/re-enable,LUT and savedCustom18 open/save/reopen |

Separate final source review covered deferred ownership, wait cancellation, exception retention, one retained worker, teardown joining, unchanged observation fingerprint, policy comparison before publishing new results, and host callbacks staying on caller threads. The pre-cancel guard was corrected and2affected targets passed again in core/SDK/ASan builds. Canonical native failure/Off/setdown check and3lifecycle/compiler checks pass; installed acceptance completed. CMake package identity now includes background_resource.hpp; runtime and observation fingerprint are unchanged.

Evidence outside Git: `work/cache-first-startup-20261020/` with check logs, mutation reports, `first-cached-pixel-check.json`, original failed host report and controlled retry. Retry8be24512bee8528d contains a test-only model-loading gate; it is not canonical product source. Its completed retry2 and accepted reference sessions have exited; exact test modules are archived and accepted9bc inventory/signature unchanged. Original strict USER_STOPPED-only harness failures are preserved: live enums confirm3015isQUEUED, with actual native abort517 and renderingfalse. Coordinate Stop attempt returned AXError; only keyboard cancellation is claimed. A first strength comparison used mismatched settings (2codes); it remains recorded as an invalid oracle, replaced by7matching-setting accepted comparisons at the unchanged1code tolerance.

GPU research is separate in `work/gpu-colour-20261020/`:30latest component cases,11strict numerical failures, alpha exact. It is neither integrated nor installed; no GPU user-speed claim. Preserve all failures and the accepted installed rollback.

## Final acceptance and limits

[Evidence hashes](CACHE_FIRST_STARTUP_EVIDENCE.json), `current/verification.json`, `matching-reference-verification.json`, `final-review.json` and `acceptance.json` bind installed16e472812ee35e41. Seven1080p gated-trial comparisons and15installed960x540 comparisons match accepted exported pixels exactly (0code difference). This is exported8bit PSD evidence, not a claim that arbitrary32bit outputs were independently compared bitwise. Render arithmetic/model/detail remain unchanged by source review. The native observation fingerprint remains17913797ac0c209448e722b56cbda5c2f39765a943415a7b3ae369c98f681a6f. All owned test sessions exited; helpers archived, current and rollback inventory/signatures verified. User projects untouched.

The full mock SDK callback fixture was mistakenly selected against a bare native build without packaged model resources: Smart pre-render assertion187 failed. Original `current/canonical-static-checks-attempt1.log` is preserved. The correct native missing-resource/Off/setdown case and3canonical lifecycle/compiler tests pass; full mock34/34 and real installed host acceptance are separate evidence. This fixture correction is not a fixed product defect.

Saved masks now avoid model construction altogether. A true miss still initializes the same full models; it is not a promise of faster first-use analysis or a universal startup/render multiplier. The M1Pro timing observations are diagnostic, not a controlled broad performance claim. GPU remains experimental and disabled because11/30strict numeric cases failed. Vendor libraries/AE are not instrumented by local sanitizers. No Windows/FPS/universal-footage qualification, installer, release or GitHub push.
