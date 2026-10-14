# FSTR-LUT — AI Enhance + Looks v3

Current scope: user's color-first MVP steering, recorded in MVP_COLOR.md, takes precedence over the original v3 sequence below. V3 remains the broader post-MVP contract unless superseded by a direct decision. Initial GitHub repository was empty; preserved baseline b21b571. Branch dev/ai-enhance-prototype. No main/release/publication authorized.

## Discovery and delta
For AE artists processing phone footage, compressed SDR, low light, moderate softness, photographs and graphics. Apply effect -> automatic moderate Enhance/Fast/Neutral -> select Look -> adjust strength. No analyze button or model selector. Preserve night/light character, faces and clean footage.
AI is now core; HDR, upscale, interpolation and generative face changes are excluded. Custom .cube moves after MVP. Six looks: Neutral, Natural Skin, Warm Film, Cool Modern, Soft Vintage, Night Mood. Five main controls: Enhance, Look, Look Amount, Quality, Mix. Advanced controls only when algorithms make them meaningful. No independent framewise auto color or previousFrame. Python/PyTorch research/export only. JSON state and embedded versioned LUTs. Fast 33–40 ms at 1080p is target, not claim. User selected this Mac as initial reference.
Stage 0 is covered by supplied contract. Model/license/quality/speed unknowns block production selection, not independent research. Senior consultation was expressly authorized, read-only, no implementation ownership.

## Accepted rules
AE Development Rules v11.1.0, commit 04b606845e0f73ab28b9807bb45682eae4d6ce34, tag object 67359cba5ff85401c90455ccde803e65dcc91a85. Complete checkout ../AE-Development-Rules-v11.1.0. Smart Entry, Product Discovery; Process §§1–13; Engineering §§14,16–21,24–25,32,34–35,41; Native §23; transfer §26. Critical risk, Development delivery. Research-methodology governs audit; production-engineering governs implementation/native review/testing. Reference Audit N/A: model research inputs are not whole-product parity targets. Release, updater, IPC, CEP, UXP, HDR out of this block.

## Architecture / first slice
C++17 core without AE headers. Typed borrowed CPU RGBA float includes dimensions, stride, accessible buffer length, transfer/primaries, alpha mode. Immutable settings, explicit FrameWindow, context resources/backend/cancel. First experimental model FFDNet Color Clip is a small denoising path; NAFNet-SIDD-width32 is a quality comparator, no production choice yet.
One ONNX Conv/ReLU trunk; exact C++ shuffle/pad/sigma prep; shared loaded session, one active inference, reusable context buffers. Temporal radius explicitly zero; other windows rejected until implemented.
Initial AI slice supports opaque SDR with explicit sRGB/linear-sRGB. Reject unsupported alpha/extended input, never successful silent clipping. Mix=0 bypass remains exact; LUT-only uses explicit residual policy. These research restrictions do not remove full v3 requirements. No fake auto color, defect estimator, independent Denoise/Detail or skin recognition.

## Acceptance / traceability
| ID | Requirement | Check / remaining obligation |
| --- | --- | --- |
| AI-01 | Exact weights/code provenance and commercial redistribution rights | Model audit; rights unresolved => no weights redistribution |
| AI-02 | Model improves defects without face/detail invention | Real pretrained inference + paired synthetic metrics; natural video review open |
| CORE-01 | Typed bounds/stride/color/memory; no AE headers | C++ compile, bounds/stride/NaN tests |
| CORE-02 | Frame -> model -> LUT -> same dimensions | Upstream/ONNX/C++ parity; odd-size/RGB shuffle |
| MIX-01 | Mix=0 exact, Enhance=0 allows looks | Extended/transparent bypass + backend call-count |
| COL-01 | Model normalization/gamma, no inferred Rec.709/global clamp | Explicit color/unsupported-input tests; AE working-space later |
| LUT-01 | Tetrahedral CPU and exterior policy | Independent six-simplex/identity/domain tests |
| STATE-01 | Versioned JSON, embedded versioned LUT, preserved old state | JSON foundation; native save/reopen/Undo later |
| TEMP-01 | Explicit bounded window, cut/boundary behavior, order independence | Radius-zero repeatability now; video/SDK neighbor checkout later |
| CACHE-01 | Look change doesn't rerun restoration, bounded invalidation | Later after content identity and AE cache audit |
| AE-01 | Native automatic Enhance/Neutral/Fast; 8/16/32, alpha, ROI, scale, cancel | Stage 2 NOT RUN until core measured |
| MFR-01 | Bounded loaded sessions/parallel inference | Shared CPU backend tests; real AE MFR later |
| PERF-01 | Cold/warm/total times, RAM/VRAM, 1080p/4K | CLI/research MPS; AE/4K later |
| VIS-01 | Clean/night/faces/textures/text/motion/compression/cuts | Authored synthetic chart now; representative video corpus open |
| UX-01 | Five controls, meaningful advanced group | Native stage 2/4 |
| PORT-01 | Shared source, actual Windows/macOS evidence | Mac C++ CLI now; Windows/AE NOT RUN |

## Sequence
1. Candidate/license/environment audit, source/checksum pins, comparative bench.
2. Minimal C++ pipeline and ONNX CPU reference, parity, timing, synthetic rendered QA.
3. NAFNet and short-video quality; CoreML EP placement/precision investigation; select only from evidence.
4. AE prototype: Enhance, Neutral + one artistic look, Look Amount, exact runtime identity and total latency.
5. Temporal/cuts, adaptive/stable color, six looks, Fast/Best, bounded caches, actual MFR.
6. Both platforms and measured optimization/fine-tuning. No release implied.

Core standard library + small MIT JSON/SHA headers; external ORT; external Adobe SDK later. Source/assets all rights reserved pending owner's license decision. No imported footage/LUTs.

## Authorized continuation block 2 — 2026-10-14

User requests maximum useful steps in one run and short human progress after each step; senior-model code consultation is explicitly authorized. Restore/preserve baseline2f6f3f2. Development branch retained; no release/main/unknown cleanup. Bounded work:

1. Real-footage technical corpus with source rights/attribution, explicit color conversion and disjoint scene train/validation/test splits; authored corruption and unmodified source distinguished.
2. Official FFDNet gray/luma and IRCNN color timed against prior RGB model; excluded candidates record actual license reason.
3. If measured speed gate fails, bounded existing-architecture residual student/distillation research (not a new large network), train recipe and exact teacher/source provenance; held-out scenes untouched until selection frozen. Unknown teacher rights keep all derived weights research-only.
4. Core improvement: conservative defect/confidence/noise gate, clean exact bypass, temporal-statistics-only explicit window and cut boundaries; no temporal pixel mixing. Tests must show benefit on degraded input, not just a no-op mask.
5. Optional bounded fast transfer implementation only after independent error bound vs reference; retain exact reference and extended-value fallback. Real CoreML C++ path, active cancellation/admission tests, source/state serialization and six original look definitions where independent.
6. AE research adapter/bundle and owned-host checks only when dependent model/core quality gates permit the stated prototype scope. Incomplete selection is not production approval. Actual AE/Windows/runtime gaps remain explicit.

After each block update STATUS or this record with measured scope, evidence and next decision. Benchmarks run serially, separate from compilation/training. Final source review and affected checks required before push to dev branch.

## User steering — quality before acceleration

2026-10-14: establish which processing actually works before making it faster. This overrides the execution order above. Park further student training, color/backend optimization and AE integration; preserve their existing changes and failed experiments. A fast near-identity student is not success.

Q-01: compare source/no processing, ordinary blur, RGB FFDNet, IRCNN and NAFNet on identical short motion clips and identical corruptions. Include original material, weak/strong added noise, extra H264 compression and modest blur. Freeze cases/seeds/settings before looking at results. Explicit sigma15 is a research setting, not automatic estimation.

Q-02: evaluate the official SIDD homepage noisy/reference pair separately as real smartphone noise; it is one illustrative published example, not independent held-out certification (NAFNet was trained on SIDD). Preserve original-size crops, range/color/detail errors and negative results. No extrapolation to all phone video.

Q-03: compare unclamped float outputs against references, gradient error and motion-relative error changes. Inspect stills and prepare motion previews; visual motion inspection is a separate gate. Natural ToS source is already compressed/graded, not noise-free GT. Metrics from authored corruptions establish only recovery of those corruptions. Clean original damage, residual color bias, no-op and blur counterexamples are explicit vetoes for universal default selection.

Q-04: document defect-specific usefulness and outstanding automatic/temporal/color/native requirements, then select the next quality experiment. Speed targets remain open; do not optimize a failed quality path.

## Current priority — color-first MVP

Direct user steering: «нам прежде всего надо решить вопрос с цветом плоских исходников, чтобы можно было работать как с логарифмом. остальные улучшения оставь на потом после мвп». Restoration/model selection, denoise, compression, detail, temporal restoration, training and acceleration are deferred until after MVP. Their code, evidence and open requirements remain preserved. Model latency does not gate the color-only MVP.

Clarification closed: both ordinary SDR grading and true camera Log processing; no SDR-to-Log export in MVP. User delegated initial profile choice. Choose explicit Sony S-Log3/S-Gamut3.Cine first, using the official Sony formulas/primaries; ARRI source research preserved without expanding current delivery. Implement deterministic host/source/output contract, complete color-only frame path and checks; then native adapter/owned-host validation. No inferred camera gamut, framewise auto-color or recovered RAW/scene-latitude claim. Details and remaining acceptance in MVP_COLOR.md.

## Authorized metadata addition — 2026-10-14

User requested automatic useful embedded camera metadata reading. Implement META-01..07 in METADATA.md as a scoped color-MVP addition: bounded independent Sony NRT MP4 reader, automatic read-only ECW recognition/recommendation, explicit existing render profile/encoding. Preserve old color checkpoints and all broader obligations. No render-time filesystem work, inferred gamut, silent Log normalization or private footage publication. Native UI12 is appended without changing diskIDs1..11. Actual source/host acceptance must bind the new candidate; old356 host evidence is a regression reference, not proof of new UI.
