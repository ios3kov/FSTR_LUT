# FSTR-LUT checkpoint — 2026-10-14

Contract: USER_SPEC_V3.txt, replacing first brief retained in USER_SPEC.txt/history/PLAN-v1.md. Baseline intake b21b571 preserved. Branch dev/ai-enhance-prototype. Accepted rules v11.1.0 / 04b606845e0f73ab28b9807bb45682eae4d6ce34. Tested core source: d30a40f35091d1a63a7a897d99a6fb8ec832a8ff. Model tooling source be42eb87ed6e0f5b29c82475c0e25c4575fcc667 is unchanged in that candidate. Evidence: ../evidence/2026-10-14-d30a40f/manifest.json, raw check/benchmark records beside it. This checkpoint/report commit is separate from tested source bytes.

## Current result

The first independent research slice exists and runs actual pretrained inference: frame -> C++ FFDNet preparation -> real ONNX CPU model -> tetrahedral LUT -> linear-light blending -> same-size output. Python/PyTorch is not linked into C++. One shared loaded CPU session, serialized inference, reusable packed/output buffers, one exact-content bounded restoration cache. Look/enhance blend changes reuse restoration; input/sigma/dimensions/backend changes invalidate it. JSON research looks embed LUT text and version metadata. Neutral and Warm Film are prototypes, not the final six-look artistic library.

Development only. Stage 1 is still in progress: neither production model nor natural-video quality is accepted. No native AE plugin under v3, no installation, no main merge, no release. Checkpoint does not remove any PLAN requirement.

## Checks actually performed

| Check | Result and exact scope |
| --- | --- |
| C++17 arm64 core + optional ORT CLI build | PASS exit0; CLI SHA256 f0170986bbe546fa9575d7474aa1db35e84114eaebfd395aef5518b10fc6a6db |
| Core contracts | PASS 2377 bounded assertions; actual source tests include stride/bounds, malformed LUT, 1000 seeded parser fuzz inputs, six tetrahedral simplexes, identity17/33/65, extended LUT values, Mix0 exact, Enhance0, explicit colors, premult LUT, cache invalidation/physical capacity and parallel Echo contracts |
| ASan + UBSan | PASS pure core suite; external ORT library not instrumented. TSAN NOT RUN |
| Upstream -> ONNX | PASS FP32 tolerance1e-5, max4.231929779e-6; odd/tiny sizes included |
| Final C++ model + Neutral LUT -> upstream | PASS max3.755092621e-6 on257x255; final binary and fixture hashes recorded |
| Model error contracts | PASS wrong checksum, path traversal, normalization and schema reject with exit1/no output; backend source unchanged since tested be42 |
| CPU -> MPS | PASS research tolerance1e-4 at512x384; FFDNet max2.384185791e-6, NAFNet max6.556510925e-7. No native GPU backend claim |
| CoreML EP | PASS graph placement profile contains CoreML provider and CPU-reference numerical tolerance1e-5. Underlying ANE versus GPU device NOT ESTABLISHED |
| JSON look asset fidelity | PASS embedded cube bytes, checksum, schema/version/color metadata match exported cube |
| Rendered synthetic chart | Inspected at1536x824 and individual512x384 panels. FFDNet cleans noise but softens small text/lines; NAFNet strongly loses synthetic text/stripe detail. This is a counterexample, not acceptance on faces/real footage |
| Natural-video quality/temporal motion/cuts | NOT RUN; five same-input static-scene noise frames do not establish motion or cut correctness |
| Active ORT cancellation/admission wait | NOT RUN; pre-cancel/core behavior tested, monitor ownership reviewed |
| Native AE 8/16/32, SmartFX/ROI/scale/alpha, save/reopen/Undo, real MFR | NOT RUN / not yet implemented under v3 |
| Windows / 4K / security release audit / full six looks | NOT RUN |

## Initial measurements — this Mac only

M1 Pro,16GiB unified RAM, macOS26.6.2. No other agent benchmark/build/test during final serial CLI timing. General desktop load is not controlled. Small samples, no stable performance certification. First frame means first execution in the created process/session, not a clean OS/device-cache cold start.

| Path | Input / settings | Observed result |
| --- | --- | --- |
| FFDNet PyTorch MPS full reference | Synthetic1920x1080,FP32,sigma15/255; includes upload/download | First257ms; warmed219ms (~4.57fps) |
| NAFNet PyTorch MPS | Same1920x1080 input | First3587ms; warmed2560ms (~0.39fps). Earlier trial OOM at same40% allocation policy; later trial passed. Memory stability not accepted |
| FFDNet ORT CoreML trunk | Fixed1920x1080,excludes C++ color/LUT/AE | Warm median239ms over3 runs; CPU max error1.132488251e-6 |
| FFDNet C++ ORT CPU full pipeline |1920x1080,32bpc,Enhance0.5,Warm Film1.0,4threads,cache disabled | Three runs3718/3815/3537ms total; peak child RSS940769280bytes (~897MiB). Includes core prep/model/LUT/mix; excludes AE/file reading/model session load |

**Fast33–40ms goal: FAIL for measured paths.** Do not connect an aggressive default automatic effect and call it ready. On the paired512x384 synthetic Gaussian test, input24.72dB, ordinary blur26.87dB, FFDNet32.66dB, NAFNet22.37dB. Both change clean synthetic input; PSNR is not a substitute for natural video review. Results cannot be generalized to real-noise model training domains.

## Separate final review

Spec review: first research slice matches explicit architecture/measurement priority. Full product remains incomplete; PLAN traces all postponed alpha/extended AI, temporal/cut/stable auto-color, skin protection, full library, presets/native portability, Fast/Best, AE/Windows and memory/performance requirements. No fake advanced controls or successful restoration fallback. Custom cube UI remains after MVP.

Quality review: root reviewed frame arithmetic, parser bounds, LUT simplex sampling, transfer/mix units, cache keys/capacity, immutable render settings, session/buffer ownership, exception boundary and cancellation paths. Authorized senior model gave independent source advice/review; read-only, no edits/tests claimed by that reviewer. Corrections: borrowed Ort TypeInfo lifetime; nonallocating error-message fallback inside noexcept; padded area budget; bounded line reading; physical cache capacity; explicit thread error propagation; color oracle coverage; removal of potential per-pixel stable_sort allocation. Rechecked affected final core/sanitizer/parity scenarios. Fault injection and active ORT cancellation remain unverified.

Initial failures/history are retained in the evidence manifest: export3e-6 threshold, faulty h1 stride oracle, initial CLI exit139 (precise causal reproduction NOT RUN), first NAFNet OOM, and denied sysctl inside time wrapper. No unsafe memory limit was removed; final timing/RSS used a serial subprocess and resource API.

## Reproduce

Pure core: `python3 scripts/check.py`; sanitizers: `python3 scripts/check.py --build build-sanitize --sanitize`. Optional CLI: configure CMake with `-DFSTR_ORT_ROOT=/path/to/onnxruntime-osx-arm64-1.23.2`, then build.

Research preparation (external pinned KAIR/source and official checkpoint required; hashes enforced): `python tools/model_export/export_ffdnet.py --source /path/to/KAIR --weights /path/to/ffdnet_color_clip.pth --output /new/research/export`. Exact model manifest is in assets/models; external model bytes are not committed. Root lockfile tools/model_export/requirements-lock-macos-arm64.txt captures this research environment, not cross-platform plugin dependencies.

Compare: `python tools/benchmark/compare_models.py --research /path/to/research --output /new/research/results --device mps --full-hd` (requires pinned KAIR and NAFNet folders plus models). CLI usage is printed by fstr_benchmark. All prototype processing is local; no render-time network.

## Next concrete work

Finish Stage1: compare a lighter same-resolution model/weights and clean-material adaptive bypass on a rights-cleared representative video corpus; confirm actual redistribution rights. Keep independent frame/short-video quality, speed and memory gates. Only then implement the minimal AE adapter and measure its complete path. Do not fix current latency by silently changing resolution, removing memory guards or asserting untested tiling equivalence.

Cleanup: only owned intermediate/build/research directories were created. They are retained for reproducibility and rollback; no uncertain material, installed plugin, project, other checkout or historical Evidence was deleted.
