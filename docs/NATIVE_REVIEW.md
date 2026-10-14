# Final focused review — metadata/color development candidate

Source8733709c7076da76a95e0510a9b55fd0cb2b7b8a; signed BuildID32597669ec8fb8e1. Root performed a separate critical review after actual UI/regression work. Current acceptance and exact evidence are in STATUS.md and EVIDENCE_COLOR_MVP.json; historical review below remains source-bound.

Reviewed current delta: complete structural ISO-BMFF XML document parsing, box/read/depth/XML limits and namespace/conflict rejection; pinned tinyxml2/license; field whitelist; UTF16path size/NUL/surrogate handling; owned AEGP memory and suites; Drawbot RAII cleanup; bounded UI-only cache and refresh; render callback isolation; appended UI-only diskID12. Render parameter acquisition remains1..11, all existing color IDs/defaults/units and core processing are unchanged. No exceptions cross EffectMain. No remaining source defect identified in this focused review.

Actual owned AE proves direct-camera automatic caption, replacement fallback, source Undo return and ordinary-panel legibility;10 fresh32-bpc project exports pass the original2/255 tolerance. Functional numeric tests, actual host behavior, visual observations and source declarations have separate evidence. The single SDR comparison looks intentionally subtle; it is an artistic judgment, not user acceptance or camera Log verification.

Remaining limits: UI filesystem reads are byte-bounded, not a wall-time guarantee for slow volumes; manual profile/encoding remains required; proxy/missing/composition/refresh/narrow actual-host checks are not substituted by mocks. Full camera numeric comparison is uncalibrated after a clipped PSD8 baseline; float export attempts fail explicitly and do not become product PASS. Windows/realLog/direct extended floatworld/actualROI/downsample/gradingUndo/RAMPreview and measured callback concurrency remain open. Header-binding-only CMake adjustment changes the final BuildID after the sanitizer run; final signed Release has its own5suite PASS.

Completed owned AE was saved and normally closed; its temporary runner is preserved outside MediaCore. No unrelated plugin/project/preferences were changed. No full MVP/release-readiness claim.

## Historical color source review — preserved

# Focused native review — color MVP

Reviewed implementation source25819a9e379a49caaa94df4fa5e2af5112b5cd07, C++17/AppleClang21/arm64/Release, external SDK25.6_61. Exact file hashes and signed artifact binding in history/EVIDENCE_COLOR_MVP-7fbe.json. Root performed this separate review after implementation; senior review was read-only, no execution. It is source/fixture review, not host certification.

| Area | Reviewed paths / established behavior | Remaining boundary |
| --- | --- | --- |
| Memory | color/frame validates nonzero dimensions, accessible count, overflow-safe stride/pixel budget; packed owned output, row-wise byte-exact bypass. Native process_world validates data/rowbytes/dimensions and pointwise origin mapping before borrowed reads/writes | AE owns buffer allocation/alignment; actual host ROI/downsample/alpha output not exercised |
| Numeric | grade/profiles/pixel: official Sony branch/units and independent gamut oracles; finite float input/double math; output float bounds, signed transfer extension, stable large shoulder. Host16 maximum32768 from exact headers, nearest integer rounding and representable clamp; float no global clamp | Extreme active encoded samples can fail explicitly; optional LUT coordinates clamp to domain. No gamut mapping or recovered camera latitude |
| Ownership/ABI | Params acquired1..11 checkout/reverse checkin, constructor partial failure cleanup; WorldSuite acquire/release; SmartFX pixel checkout/checkin on error/exception; cleanup error reported. SDK declares layouts/calling conventions; C ABI catches native PF_Err and C++ failures, enum throws cast to PF_Err | Real host callback ordering, host-suite error semantics and cancellation remain NOT RUN |
| Concurrency | Immutable frame settings and shared frozen LUT snapshot; one initialization mutex, atomic shared_ptr load/store; render has no global previous-frame state or filesystem/model operations; parallel fixture frames agree | Host lifecycle/MFR declaration is not execution proof; no TSAN run |
| Units/state | Native ARGB fields converted individually to normalized float RGBA, premultiplied stored-sample contract; exposure stops, UI percent-to-unit strengths, creative balance percent-to[-1,1], explicit host transfer. SmartFX origin layer-coordinate difference without render-scale multiplication. DiskIDs1..9 preserve first prototype meaning, new10/11 independent of display order | AE premultiplication/host encoding and actual save-reopen persistence must be confirmed; camera override only for preserved unconverted full-range Log |

Actual executed checks: final Release3433core/2104color assertions;78 assertions in actual dlopened bundle fixture; corresponding ASan/UBSan fixtures;9 raw full-frame cases with independent synthetic known encodings, max4.30931e-7. Final face/night/rooftop composites inspected at1536x1044. SDK flags, PiPL data fork, exports, dependencies and strict ad-hoc signature inspected. Actual AE loaded a mapped identified module; script/registry/render was not established. -noui route failed in a host menu/error stack; cause remains undiagnosed, negative log retained. Locked Mac prevents ordinary UI acceptance.

Excluded from this source review: parked AI/inference/training/backend and model licenses, GPU parity (no native GPU path), full artistic look/preset library, Windows packaging and full release/security/notarization scope. These are preserved obligations or later work, not implied PASS.
