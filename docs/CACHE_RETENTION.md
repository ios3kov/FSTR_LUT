# CACHE-RETENTION-001 — short-clip replay, 2026-10-18

The real Nikon Z6 evaluation retained only64 of72 raw-frame analyses (244MiB). The count cap evicted masks before the512MiB byte budget was reached. A deterministic public-cache72-frame restart/replay regression fails on the previous implementation with repeated compute calls.

Disk retention now allows4096 complete entries while keeping the existing512MiB logical payload budget. The count bounds tiny-entry filesystem overhead; it is not a frame-count retention promise. Eviction remains oldest-write, not LRU; RAM remains four entries/128MiB. No schema, analysis key, pixel math, LUT, model, UI or project-ID changes. Existing cache entries remain readable. Larger-than-budget traversal can still evict and recompute masks. Directory/header/filesystem overhead, temporary writes, caller-retained buffers and model resources are outside the payload limits.

Acceptance:72 distinct frames compute once and survive an empty-RAM restart/replay; existing cancellation/coalescing/identity/corruption/recovery contracts pass; an oversized sparse oldest managed payload is evicted without removing the current valid entry; foreign paths preserved. Real AE restart plus Smart Off/On and LUT-strength reuse must be recorded separately from the fake-compute regression.

## Result

PASS:14Release and14ASan/UBSan targets, followed by both extended cache targets including the4096-entry boundary,72-frame restart/replay,513MiB sparse-payload eviction and existing recovery/identity contracts. The pre-fix72-frame regression fails with “72-frame disk replay repeated analysis below byte budget”. Static scanner has no findings in its scanned scope; it does not assess release readiness.

PASS actual AE25.6 on this Mac, Current9838bb4d38c6e9c9:217opaque960×540 exports. Restart with the previous64-entry evaluation cache performs8computes plus222disk/124RAMhits to fill the missing frames. SmartOff performs no analysis accesses. Onagain:0compute/236disk/118RAMhits across72frames. Strength65→80:0compute/228disk/126RAMhits across72frames; output changes as intended. Repeat same-settings output differs by at most one8bit code. All72 entries retained,287385095logical bytes, below512MiB.

The test intentionally purges AE caches between stages, so the results establish native analysis reuse rather than only AE rendered-frame reuse. Prior cache schema/model/policy remains readable in the restarted host. No claim that analysis or rendering is now real-time. Long clips beyond the byte/count budget can still recompute. General professional grading quality, all skin tones and continuous visual acceptance remain separate; no new visual-quality claim from these numerical checks. LUT-selection reuse was already exercised in CACHE_RECOVERY; this host sequence newly checks strength changes.

Owned AE exits normally; exact helper archived. Installed signature/current/prior inventories verified; previous Current preserved at `../cache-retention-20261018/previous-current.plugin`. Models/runtime unchanged. No installer, release, push or merge. [Evidence](CACHE_RETENTION_EVIDENCE.json).
