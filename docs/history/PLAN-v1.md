# FSTR-LUT: product contract and implementation plan

Source: full user specification in USER_SPEC.txt, accepted 2026-10-14. No prior implementation exists. Repository `ios3kov/FSTR_LUT` was empty when cloned; no existing changes were overwritten. Work occurs on `dev/sdr-foundation`, not main.

## Product discovery

For After Effects artists grading ordinary SDR footage locally. Apply the effect, select the actual incoming working-space contract, adjust exposure/contrast/highlights/shadows/saturation/creative balance, choose a bundled or imported look, blend and save. The value is predictable grading with portable LUT state. There is no RAW/LOG/detail recovery claim. Unknown LUT color spaces require explicit metadata, not automatic detection. Errors must be visible and must not silently substitute a look. User specification covers Stage 0; the exact repository URL was the only blocking intake question and was answered.

Core v1 includes native macOS arm64 and Windows x64, 8/16/32 bpc, CPU reference, custom .cube, portable presets and six original looks. GPU/MFR validation follows the CPU reference; HDR follows completed SDR. No AI/cloud/temporal dependence. Precise AE/OS compatibility and speed budgets are measured, not assumed.

## Rules baseline

AE Development Rules v11.1.0 at `04b606845e0f73ab28b9807bb45682eae4d6ce34`; annotated tag `67359cba5ff85401c90455ccde803e65dcc91a85`. Complete checkout is ../AE-Development-Rules-v11.1.0. Adoption selects AI_ENTRYPOINT, Product Discovery, Process §§1–4,6–13; Engineering §§14,16–21,24–25,32,34,41; Native §23; transfer §26. Risk Critical (render/math/serialization); delivery Development. Reference Audit N/A: user supplied a specification, not a product parity target. Release/distribution, IPC, updater, AI, CEP and UXP are outside this development block. Use production-engineering's workflow, native and behavior-testing guidance. No agent delegation requested or used.

## Acceptance and traceability

| ID | Requirement and observable acceptance | Block/check |
| --- | --- | --- |
| SDR-01 | Neutral controls and disabled LUT return exact original including transparent/extended pixels | CPU neutral tests; AE depth fixtures later |
| SDR-02 | All controls have bounded finite values and documented formulas/defaults | CPU analytic control tests; COLOR.md |
| LUT-01 | .cube 17³/33³/65³, domain validation, red-fastest order, tetrahedral results | Parser malformed/size/domain tests; six simplex tests |
| LUT-02 | Outside-domain policy preserves extended values; no global [0,1] clamp | Identity/affine/exterior tests |
| LUT-03 | Six original looks, descriptions, fixed spaces, strengths and limitations | Asset checks + rendered visual review |
| COL-01 | Explicit native/linear/sRGB input; no inferred Rec.709 or double gamma | Color roundtrip/contract rejection tests + AE linear fixture later |
| ALPHA-01 | Preserve alpha; unpremultiply/repremultiply; transparent policy | CPU alpha tests; AE transparency later |
| STATE-01 | Versioned presets embed LUT bytes/metadata; no path dependency | Roundtrip/error tests; AE arbitrary-data import/save/reopen remains open |
| AE-01 | Native SDK adapter + bundle builds, flags/PiPL align, exceptions contained | SDK build and binary audit; actual host load/render open |
| AE-02 | 8/16/32, AE16=32768, row stride, ROI, scale, cancellation, parameter checkout cleanup | Adapter code review + actual AE fixtures open |
| MFR-01 | Deterministic parallel/out-of-order results without mutable render state | Core parallel tests; actual AE MFR open |
| PERF-01 | CPU prototype time at 1080p/4K; AE/RAM/VRAM/cold/warm separately measured | CPU benchmark; AE profiling open |
| GPU-01 | Select SDK-supported backend, CPU parity tolerance before implementation | Stage 3, NOT RUN |
| HDR-01 | Explicit nits/white/peak mapping and AE output workflow in float | Stage 4, NOT RUN |
| VIS-01 | Gradients, skin, night, saturated, highlights, compressed 8-bit, cuts | Synthetic swatches now; footage and AE viewing later |

## Sequence and architecture

1. Audit/toolchain/color contract (docs/AUDIT.md, COLOR.md).
2. Host-independent C++17 processing core, color pipeline, LUT engine, portable preset serialization and CPU backend. Tests first for dangerous math/parser behavior.
3. Native SDK adapter with built-in controls/looks, SmartFX, no render-time UI/file access. SDK headers are external and never copied into Git.
4. Native custom LUT/preset UI and arbitrary-data state, save/reopen/Undo/portability fixtures. This remains an obligation even if the initial core and bundle build pass.
5. Owned AE runtime: exact loaded identity, neutral/non-neutral 8/16/32, linear space, transparency, ROI/downsample, RAM Preview, queue/aerender, cancellation, MFR, save/reopen and error paths. Windows requires its own toolchain/host.
6. Measure, then GPU; finally HDR. No scope reduction or release inferred from this initial block.

## Dependencies/licensing

C++ standard library only for core; CMake/Clang for local build; Adobe SDK external under Adobe's terms. New LUTs are authored analytically for this project. Product source/assets remain all rights reserved pending owner's license decision; do not invent an open-source grant. No imported media or third-party LUTs.
