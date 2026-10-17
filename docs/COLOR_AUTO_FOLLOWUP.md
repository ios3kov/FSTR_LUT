# Color follow-up — 2026-10-17

## AE-AUTO-001

Requested: Auto by default for new effects; manual encoding inside collapsed Advanced. Preserve old disk ID9 and popup values1–4, old projects and render determinism. Unknown/unsupported AE encoding must not silently become sRGB. Accept only when project working-space and Linearize changes, MFR, save/reopen and fresh aerender resolve current input encoding correctly.

Implemented slice: existing encoding selector is inside collapsed Advanced. Topic IDs14/15 are new; old persistent IDs unchanged. Renderer excludes group markers from checkout and balances acquired value parameters on failure. Auto is **not implemented**: SDK ColorSettings is AEGP/main-thread; sequence setup and render may run on workers. A cached main-thread snapshot alone has no established freshness guarantee after project settings changes or in fresh aerender. No guessed default or renamed manual choice shipped. Host migration/UI still NOT RUN.

## SMART-SKIN-TONE-001

User requires adaptation to the original skin tone of each person, including exposed skin beyond faces. Skin should follow the selected look while preserving differences between people, skin texture and illumination. Do not force a common target hue, brighten dark skin to a common target, or infer ethnicity. Existing shared color formulas do not establish this feature.

Planned processing: associate reliable skin evidence with each person; estimate robust chromatic/luminance distributions per shot from explicit frame context; use bounded preset-conditioned residuals per person, modulated by light/shadow and confidence. Reject highlights/clipping and uncertain occlusion from reference estimates. Small/blurred people retain coarse soft fields. Uncertain identity must not exchange reference statistics between people. Long unreliable gaps fade to ordinary grading. No hidden previous-frame state; analysis/cache key includes algorithm/settings/reference inputs, excludes look/strength when those affect color only.

Acceptance: mixed light/medium/dark tones within one shot, different illumination of the same person, visible arms/hands, crossings/occlusion, approaching/receding and cuts. Verify no forced tone convergence, no bleaching of dark skin, no false warm correction under colored light; compare motion stability against source and ordinary LUT. Report numerical checks separately from visual acceptance. Current implementation and representative diverse-tone visual validation: NOT RUN.

## Verification of Advanced slice

PASS: arm64 native build and all10 `build-smart-native` CTest targets; all10 `build-native-sanitize` ASan/UBSan targets. Callback fixture rejects group checkouts and injects failures both after first marker and after second marker, confirming value-only balanced cleanup. Persistent IDs, four legacy choices and old default verified by schema fixture. Separate source review and `git diff --check` passed. Production-engineering code scanner reports no findings in scanned scope (619 text files); runtime certification not assessed. Actual AE UI, old-project migration and Auto: NOT RUN / not implemented. Installed effects unchanged.
