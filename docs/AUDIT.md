# Technical and model audit — 2026-10-14

## Environment / baseline

Empty GitHub ios3kov/FSTR_LUT, no prior files/commits. Initial intake records preserved as b21b571. Work is on dev/ai-enhance-prototype. No other checkout or installed plugin was changed. SDK: external /Users/os3kov/Documents/AE-SDK-25.6.KQXgVP/ae25.6_61.64bit.AfterEffectsSDK. Local host metadata: AE 25.6.0 build 25.6.0.101; actual plugin runtime NOT RUN. macOS 26.6.2 build 25G83, Apple M1 Pro, 16 GiB unified RAM, Apple Clang 21.0.0, C++17/CMake. MPS is available outside sandbox; sandbox GPU probe initially returned false. User explicitly selected this Mac as benchmark reference.

| Platform | Current evidence | Compatibility |
| --- | --- | --- |
| macOS arm64 C++ core/CLI | Build, pure contracts, actual ONNX CPU model tested | Limited to independent CLI; no AE claim |
| macOS AE 25.6 | External headers/samples available | UNKNOWN: native adapter not yet implemented under v3 |
| Windows x64 | Shared C++17 source; no local Windows toolchain/host | UNKNOWN / NOT RUN |

Do not generalize AE support to 2023/2024 or future versions. SDK is externally required for the later adapter, never redistributed. PF_MAX_CHAN16 in the actual header is 32768; 32-bit SmartFX, suites/checkout/flags and temporal neighbor requests require their own implementation/API audit.

## Candidates and decision scope

| Model | Evidence inspected / actual experiment | Decision |
| --- | --- | --- |
| FFDNet Color Clip | Official KAIR color configuration: 12 conv, 96 features, 852108 loaded parameters; input sigma stddev/255; same resolution. Official v1.0 weights downloaded, strict weights-only load, ONNX trunk exported. | First denoising research path, not universal Enhance or final production selection |
| NAFNet-SIDD-width32 | Official SIDD config: width32, encoder [2,2,4,8], middle12, decoder [2,2,2,2]; 29159715 loaded parameters. Official checkpoint; exact upstream architecture and LayerNorm classes, no unrelated BasicSR install. | Real-noise quality comparator. Synthetic Gaussian chart does not establish natural-noise performance. No ordinary tiled equivalence because global pooling |
| Real-ESRGAN | Official inference configurations/utilities inspected through senior consultation: x4 native output and later resize for outscale=1. | No runtime trial in this block; native upscale/GAN details conflict with first same-resolution non-generative priority |
| BasicVSR++ | Official backbone inspected through senior consultation: temporal propagation, warping, SPyNet/deformable conv. | No runtime trial; bounded-window/export/weights audit required before temporal selection |

Primary sources inspected 2026-10-14: [KAIR](https://github.com/cszn/KAIR), [color model](https://github.com/cszn/KAIR/blob/fc1732f4a4514e42ce15e5b3a1e18c828af47a1e/models/network_ffdnet.py), [official inference](https://github.com/cszn/KAIR/blob/fc1732f4a4514e42ce15e5b3a1e18c828af47a1e/main_test_ffdnet.py), [weights release](https://github.com/cszn/KAIR/releases/tag/v1.0), [NAFNet](https://github.com/megvii-research/NAFNet), [SIDD config](https://github.com/megvii-research/NAFNet/blob/2b4af71ebe098a92a75910c233a3965a3e93ede4/options/test/SIDD/NAFNet-width32.yml), [Real-ESRGAN](https://github.com/xinntao/Real-ESRGAN/blob/master/inference_realesrgan.py), [BasicVSR++](https://github.com/ckkelvinchan/BasicVSR_PlusPlus/blob/master/mmedit/models/backbones/sr_backbones/basicvsr_pp.py).

## Rights and dependencies

KAIR code MIT; NAFNet repository retains MIT and BasicSR Apache-2.0 terms. Real-ESRGAN code BSD-3-Clause, BasicVSR++ code Apache-2.0 (source audit, not an assessment of unrelated assets). For downloaded checkpoints, a separate explicit commercial-redistribution grant was not established by inspected material. Keep checkpoints/ONNX external, research only; repository license title alone does not close weights clearance. No user distribution/release is authorized or prepared here.

C++ optional dependency: external ONNX Runtime 1.23.2, MIT, arm64 package from official Microsoft release. Vendored nlohmann/json v3.12.0 and PicoSHA2 v1.0.1 MIT headers and notices; exact digests in evidence dependency records. Python 3.12.14, torch2.8.0, onnx1.19.0, onnxruntime1.23.2, numpy2.2.6, Pillow11.3.0, gdown5.2.0 in isolated research environment; transitive lock is preserved. They are not plugin dependencies. No claim of completed vulnerability/release audit.

[ORT v1.23.2](https://github.com/microsoft/onnxruntime/releases/tag/v1.23.2), [CoreML EP contract](https://onnxruntime.ai/docs/execution-providers/CoreML-ExecutionProvider.html), [Adobe developer entry](https://developer.adobe.com/after-effects/).

## Numerical/export choices

Export only Conv/ReLU trunk; exact C++ pre/post validated against complete upstream forward. Model session loads once per shared backend; one active CPU inference, four worker threads, reusable packed/output buffers. First cache is one bounded exact-content restoration snapshot; look/enhance blending excluded from key, sigma/input/dimensions/backend identity included. Cache byte budget measures retained capacity, not just size; working buffers and ORT arenas are separately constrained by the prototype pixel budget, not advertised as a total-process memory cap.

Initial proposed FP32 export tolerance 3e-6 failed at 255x257 with 4.231929779e-6. Reconsidered FP32 convolution accumulation tolerance: 1e-5 for same upstream/ORT FP32 reference. Actual max recorded; no claim of bit exact AI. Mix=0 remains bit-exact bypass. CoreML/FP16 must receive separate measured tolerance; do not silently treat a provider's precision as FP32.

## Selection gates still open

Natural-video corpus: day/night/faces/hair/fabric/leaves/text/motion/compression/cuts/clean. Compare input, non-AI, base model and adapted result under motion. Synthetic chart/static independent-noise probes are useful counterexamples, not video acceptance. Neither model is production-selected. Confirm weights rights; find a speed/clean-preservation path before AE UI. AI transparency/extended range, temporal neighbors/cuts/auto color, skin protection, six final looks, presets/native portability, both platforms, full AE timings and real MFR remain required by PLAN.md.
