# SMART-COLOR-REUSE-001 — reuse the ordinary look

Baseae0fd5a, dev/ai-enhance-prototype, 2026-10-18. After the cache improvement, user authorized continuing elimination of redundant color work. This step preserves accepted42ee quality and the new8entry/128MiB cache, with no controls/profile/mask/model change, approximation, GPU backend, installer or release.

## Contract and implementation

C1. Preserve numerical output and error/bypass behavior. SkinToneLook and SceneLook previously recomputed the same ordinary LUT result through nested scene/person/stable branches; AE separately computed the same plain result again. Private helpers now receive the already float-rounded ordinary array within its owning immutable look context. Original casts, branch/validation order, luminance correction, gamut limits and accumulation order remain. Zero-weight people still validate references; stable.empty/support0 do not skip current-field validation. Binding stays in the original owning SkinToneLook.

C2. SceneLook::stable_residual computes the same literal double(styled[c])-ordinary[c]. The AE adapter retains delta==0 skipping, Mix, decode/encode and premultiplication arithmetic. It removes only the duplicate scene_plain processor. A single LUT evaluation applies to this Smart residual subgraph, **not** the entire AE pixel: the main ColorPass is separate. Reuse assumes a deterministic immutable LUT/OCIO CPU transform, as supported by the existing LUT contract; no shared mutable memoization/scratch is introduced.

C3. Independent golden oracle, targeted behavior tests, sanitizer tests and separate review. C4. Actual matched old/new host processing and exact canonical-installed verification with preserved signed prior bundles.

## Independent numerical evidence — PASS scoped

Before editing, compiled and froze a legacy executable against the existing7be static libraries.18look sources (None+17cube assets), strengths0/.65/1/3,0/1/2/4/16people and128cases cover SDR/extended values, alpha0/fractional/opaque, blacks/whites/edges/NaN and invalid role/person/stable fields.46,080rows produce276,480output/error records, including old-method residual subtraction. Exception category and message hash are included, not only pixels.

The optimized executable produces all7,004,160bytes identically. Both SHA256:1ea6867730942e3d9b3144c23f0f8708422d962fe40e1e2c9a84b9e1cda657ae. Frozen executable, source, library hashes, manifests and binaries are in `../../smart-color-reuse-20261018/`. This is a finite representative oracle, not every possible input.

20Release and20ASan+UBSan tests pass. New counted deterministic sampler proves one LUT evaluation for the Smart residual even with16people, and covers residual precision, zero-weight null/foreign bindings, stable shortcuts, extended SDR fallback, Off and zero strength. Existing scene, individual tone, alpha/range, native callback, cache and external LUT contracts remain passing. Separate read-only senior review finds no concrete defect; its review is not a substitute for execution.

## Performance evidence — PASS scoped

Independent CPU residual-only microbenchmark:32,768prepared pixels, one warmup then8measured samples per case. Median old→new ms:26.034→8.063 (0people),48.821→13.514 (2),213.521→42.792 (16):3.23/3.61/4.99× for this subgraph only. Main color pass, masks, hashing, model and host costs are excluded.

Own paired AE93992 loads previous7be and distinctly named trial25368f5ce1f316f7. Same frame35/960x540/32bpc, TealOrange, identical strengths57.125/57.25/57.375, alternating old/new after warmup. Every measured render has5RAM hits, zero disk and zero inference: these are actual processing passes, not AE output-cache hits. Median whole-render time1764→1238ms,29.82%less elapsed time (1.425× throughput for the measured single-frame workflow). Three paired samples are not a production playback/hardware guarantee.

All15trial exports complete. Four old/new comparisons (warmup+threepairs) and seven accepted42ee OFF/ON golden comparisons pass <=1PSD8export code. The trial/cache helper is archived after normal own-process exit. A launcher-only Python syntax error occurred before any mutation/process launch; failed script is retained, newline corrected, then the owned trial ran normally.

## Installed working version — PASS scoped

Signed canonical **ed9e33d687019dc1** is installed under existing Current filename/match; FSTR Grading in FSTR Effects, controls/IDs/defaults unchanged. Previous7be full bundle is preserved at `../../smart-color-reuse-20261018/current/previous-installed/FSTR_LUT_AutoSmart_Current_20261018.plugin`. Accepted42ee remains in the prior cache task's current/previous-installed archive.

Own canonical AE94954 then completes12moreexports/9accepted-baseline comparisons <=1code. It loads24raw disk analyses from copied validated trial cache with zero total inference; three LUT/strength changes each use5RAM hits, zero disk and zero inference. Own project saved as Color-Reuse.aep, own AE exits normally, helper archived. Installed ed9 inventory/signature and archived7be inventory/signature are verified; accepted42ee remains unchanged in its separate archive.

Evidence: legacy-oracle.json/vector-comparison.json, bench-summary.json, ctest-final.log/ctest-sanitize.log, host-verification.json, current/host-verification.json, installation/cleanup receipts and final-review.json.27actual exports total,16accepted-baseline comparisons plus4paired comparisons; distinguish these categories.

## Remaining work

C1..C4 close for inspected scope. Full per-frame source hashing/preparation and scene-model inference remain expensive. Actual playback FPS, other hardware/resolutions, universal raw/Log recovery and GPU/FP16 accuracy are NOT_RUN. This optimization changes neither masks nor the accepted look; no new visual acceptance is inferred for untested material.
