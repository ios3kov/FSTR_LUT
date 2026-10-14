# FSTR-LUT: AI Enhance + Looks

Native After Effects effect in development under AE Development Rules v11.1.0. Current result is an independent C++ research pipeline; no accepted AE plugin build yet. See docs/PLAN.md and docs/STATUS.md for completion/open checks; full product contract docs/USER_SPEC_V3.txt.

Core build/tests (CMake 3.20+, C++17):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Optional model CLI needs external ONNX Runtime 1.23.2 and exported research artifact: configure -DFSTR_ORT_ROOT=/path/to/onnxruntime-osx-arm64-1.23.2.
Python/PyTorch is only tools/model_export, never linked into C++. Model weights/runtime are not committed/redistributed. Commands/evidence: docs/STATUS.md.

Product code/assets all rights reserved pending owner licensing decision. Third-party notices in third_party. No render-time network calls.
