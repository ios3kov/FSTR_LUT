# FSTR-LUT — AI Enhance + Looks v3

Canonical contract: USER_SPEC_V3.txt and user's implementation guidance, 2026-10-14. V3 supersedes USER_SPEC.txt; original plan preserved in history/PLAN-v1.md. Initial GitHub repository was empty; interruption occurred before processing code existed. Preserved baseline b21b571. Branch dev/ai-enhance-prototype. No main/release/publication authorized.

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
