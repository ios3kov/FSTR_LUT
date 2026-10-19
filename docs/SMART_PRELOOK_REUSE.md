# SMART-PRELOOK-REUSE-001 — exact prepared pixel reuse

Base fecfcb8, previous installed aa3b8f484020b1da retained exact. User requested continued safe acceleration. Installed canonical **ba8c5bd516081561**, FSTR Grading / FSTR Effects, existing Current match and controls unchanged. No installer/release/model redistribution.

## Implementation / P1–P2

The main ColorPass and Smart preparation previously repeated AutoSDR on the same raw pixel. Extracted the existing render-local ColorPass to SDK-free src/ae/color_pass.hpp. For active analysis, Source Host, neutral manual grade, shoulder0 and **mix>0**, capture the original float-rounded encoded premultiplied pixel immediately after AutoSDR and before the main look/final Mix. A separate prepare processor with no look and Mix1 would bypass-return that exact array. Without active AutoSDR it would return raw. Other manual/Log/shoulder settings retain prepare.apply. Subsequent alpha division, transfer decode, masks, skin/scene residual, main grading and quantization keep their original arithmetic. No shared mutable memoization, new threads, approximations, model or mask change.

Capture requires can_share_prelook(); invalid use throws rather than returning an incorrectly prepared value. Mix0 is explicitly ineligible, although active analysis already excludes it. Output is unspecified on exception and never read after failure. Premultiplied alpha and hidden RGB retain the old paths.

Frozen previous ColorPass is an independent fixture, using unchanged color core. **604890 comparisons** cover main/prepared bits and exact error categories/messages:8source profiles,3host transfers, fractional/zero/full Mix, manual fallback/shoulder, active/neutral/null/amount0/invalid recipes, fractional/zero/invalid alpha, hidden/nonfinite/extended samples, no LUT/zero strength and invalid protection.16800eligible captures in the main matrix. An isolated counted apply performs exactly1LUT evaluation. Separate read-only review found no material implementation defect; identified bypass/counting gaps were corrected.

Real960x540 frame35 isolated prototype,5measured repetitions after warmup: main+preparation+decode median293.237→210.865ms (28.09%less for that component).16,588,800main/prepared bytes match exactly, SHA bb30033848c711fc1115a5ff964bf32d5cfe2feb85d48837818e574a10a9d725. This excludes residual/masks/model/AE and is not the product speed claim.

## P3 native checks and corrected dependency configuration

25Release tests pass, followed by refreshed prepared_pixel_reuse after explicit bypass tests were added.25ASan+UBSan tests pass with consistently instrumented OCIO/pystring/Imath/yaml-cpp, without suppressions.24core/Cube sanitizer tests also pass. Source scanner exit0/no findings in scanned scope; does not certify release readiness.

Initial ASan job incorrectly combined sanitized product with Release OCIO and reported libc++ vector container-overflow during external LUT load. Frozen previous canonical reproduces the same failure. Existing EXTERNAL_LUT.md already describes this mixed-instrumentation issue; corrected fresh build uses existing ocio-sanitize-install and its dependency prefix. Initial failure and old-code reproduction remain preserved. No production vendor source/library was modified. Apple system/ORT libraries remain instrumentation gaps.

## P4 actual AE and performance

Paired trial **e690d22ea4cde03b** versus installed aa3b, one AE process on M1Pro16GiB, macOS26.6.2, AE25.6. Common original resources and copied compatible analysis cache. Warmups precede3pairs at distinct strengths, with order reversed in the middle pair. Every measured request executes5RAM hits,0disk,0inference, both model contexts READY; these are real processing passes, not whole-output cache hits. No own builds/benchmarks overlap measured pairs.

Median actual960x540/32bpc repeated render **756→639ms:15.476%less elapsed time**. Individual old1004/747/756ms, new625/639/670ms. Three pairs establish this fixture/workflow only, not1080p, playbackFPS, other hardware or a promise for fallback settings. Installed packaging differs in match name/source GUID, not arithmetic.

First job24successful exports/16pixel comparisons: paired warmups/threepairs, accepted shots/cuts, Mix37, manual Exposure-.125 fallback and native8/16; <=1PSD exportcode. The overall job then stops at its first alpha fixture on **old aa3b**, before trying the candidate. That failure is not counted as success. Separate alpha job verifies the same native SceneMap516/RQ3018 rejection on both versions, then8successful supported-range RGBA exports/4paired comparisons, including Mix37 and native8/16/32. Alpha decodes0..255 from4-channelPSD; RGB/alpha comparisons <=1code. This preserves the existing SDR input restriction and does not extend arbitrary alpha/HDR acceptance. Initial alpha failure root is not claimed beyond the observed baseline rejection/scene-map stage.

## Canonical installed result

Installed **ba8c5bd516081561** under existing Current path/match, default resources/cache paths, same model policy. Separate own AE completes **15exports/15comparisons** against exact previous aa3b outputs <=1code, including8shot/cut frames, Off/toggle/LUT, native8/16 and saved Custom18 open/save/reopen. **0new model inferences throughout**: the color-only update reuses compatible persisted masks. Toggle and LUT requests reuse5cached observations; Off performs no analysis. Own projects saved separately, all own AE instances exit normally; trial/helper bundles archived after inventory checks. Current and prior aa3b signatures/inventories verified exact. CPU1c and user-accepted42ee remain in their earlier archives.

47successful AE exports across scoped jobs (24partial+8alpha+15canonical),35paired/reference pixel comparisons. Deliberate rejected alpha exports are separate. Current source differs from frozen canonical only by later documentation. Separate final visual review inspected the actual installed on-35 render at960x540 and the supported32bpc RGBA gradient composited over gray: no visible spatial seams, hard mask edges or gradient discontinuities in these stills. This does not establish temporal behavior; continuous viewing is not rerun for an exact-math optimization preserving accepted references. Review PNGs remain in the task root.

Receipt: ../smart-prelook-reuse-20261019/current/installation.json. Exact aa3b rollback: current/previous-installed/FSTR_LUT_AutoSmart_Current_20261018.plugin in that task. Artifacts include baseline.hpp/source-binding, bench-verification, partial-host-verification, alpha-verification, current/verification, corrected/failed sanitizer logs, source snapshots, package inventories and cleanup receipts.

Initial Smart/context/host preparation can still take time; this change speeds the eligible repeated color pass. No new analysis/quality model, installer, merge/push/release, Windows or universal footage qualification.
