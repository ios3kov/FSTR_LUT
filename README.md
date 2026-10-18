# FSTR-LUT: color-first MVP

Native After Effects effect in development under AE Development Rules v11.1.0. Current user priority is useful color grading of flat material, before any restoration or acceleration. See docs/MVP_COLOR.md for the current scope, docs/PLAN.md and docs/STATUS.md for acceptance/open checks. AI Enhance research and the broader v3 contract are preserved for after MVP. The SDK-independent C++ color core supports SDR and explicit Sony S-Log3/S-Gamut3.Cine, ARRI LogC3 EI800/AWG3, LogC4/AWG4, Panasonic V-Log/V-Gamut, ACEScct, ACEScg and 3D Linear RGB inputs. Input Profile is inside collapsed Advanced. New effects default to sRGB output encoding; manual alternatives remain in Advanced. Natural, cinematic looks and native experimental Smart are covered by their separate evidence. The current owned macOS trial bundles native Smart; standard builds keep that optional backend off. No Python is embedded. Host acceptance remains separate from core/callback tests; see STATUS for actual results. [Input profiles and metadata scope](docs/INPUT_PROFILES.md).

Core build/tests (CMake 3.20+, C++17):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Native development build and input-space instructions: [SDR/Log guide](docs/SDR_LOG_GUIDE.md). The SDK stays external. `fstr_color_render` is the standalone raw-RGBA32f reference tool.

Optional historical model CLI needs external ONNX Runtime 1.23.2 and exported research artifact: configure -DFSTR_ORT_ROOT=/path/to/onnxruntime-osx-arm64-1.23.2.
Python/PyTorch is only tools/model_export, never linked into C++. Model weights/runtime are not committed/redistributed. Commands/evidence: docs/STATUS.md.

Product code/assets all rights reserved pending owner licensing decision. Third-party notices in third_party. No render-time network calls.
