# FSTR-LUT checkpoint

2026-10-14. User v3 supersedes first brief; baseline b21b571 preserved. Branch dev/ai-enhance-prototype. Development only; no release or native AE acceptance.

Implemented: independent C++17 typed CPU frame contract; exact FFDNet pre/post; real ONNX CPU backend; strict model manifest/checksum; deterministic radius-zero pipeline; tetrahedral .cube CPU reference; linear-light blending; Mix=0 bypass; one bounded exact-content restoration cache; cancellation plumbing; research Neutral/Warm Film v1 with embedded JSON LUT text; comparison/export/benchmark tools.

Final source review/checks are in progress. Results will be bound to source commit and individual artifact hashes in evidence. Preliminary actual GPU comparison: FFDNet 1080p around216ms, target Fast not met; NAFNet 1080p hit explicit 40% MPS memory allocation budget. Natural-video acceptance, production model selection and all AE/Windows checks remain open. See PLAN.md/AUDIT.md/COLOR.md for criteria and limitations.
