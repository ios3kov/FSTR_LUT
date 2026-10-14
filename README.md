# FSTR-LUT: color-first MVP

Native After Effects effect in development under AE Development Rules v11.1.0. Current user priority is useful color grading of flat material, before any restoration or acceleration. See docs/MVP_COLOR.md for the current scope, docs/PLAN.md and docs/STATUS.md for acceptance/open checks. AI Enhance research and the broader v3 contract are preserved for after MVP. Current result is an independent C++ research pipeline; no accepted AE plugin build yet.

Core build/tests (CMake 3.20+, C++17):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Optional model CLI needs external ONNX Runtime 1.23.2 and exported research artifact: configure -DFSTR_ORT_ROOT=/path/to/onnxruntime-osx-arm64-1.23.2.
Python/PyTorch is only tools/model_export, never linked into C++. Model weights/runtime are not committed/redistributed. Commands/evidence: docs/STATUS.md.

Product code/assets all rights reserved pending owner licensing decision. Third-party notices in third_party. No render-time network calls.
