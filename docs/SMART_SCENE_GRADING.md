# Smart scene grading — research and revised implementation contract

2026-10-15. Source baseline `75fa1ed`, branch `dev/ai-enhance-prototype`. Direct user steering: automatic discovery; actively grade skin within the chosen look, not preserve original skin; apply scene understanding to ALL frame objects. No manual object selection. Quality before speed. This supersedes a face-only endpoint; no claim of implemented semantic scene grading.

Current implementation and executed checks: [SMART_SCENE_IMPLEMENTATION.md](SMART_SCENE_IMPLEMENTATION.md). Initial statuses below describe the research checkpoint and are superseded by the scoped implementation evidence.

## Research decision

Adopt a coherent scene look plus bounded, confidence-weighted material/region adjustments. A class label alone cannot determine the desired color. A red coat is not automatically orange; blue light on skin is not automatically a white-balance error. Keep illumination, relative complexion, texture, highlights and inter-object relationships. Automated estimates are uncertain evidence, not ground truth. No universal skin target, no per-frame gray-world normalization and no independent hard recoloring of every detected object.

The initially drafted fixed per-look temperature/tint skin branch is insufficient for this request. Its unvalidated patch/recipes are preserved outside the product in `work/skin-toning-2026-10-15` relative to the task root. Runtime source remains exactly baseline; installed plugins were not changed.

## Sources actually inspected

All accessed 2026-10-15. Product documentation describes intended tools, not independent proof of quality or disclosed proprietary algorithms. We are borrowing principles, not claiming Resolve/Baselight parity.

1. [Blackmagic, The Colorist Guide to DaVinci Resolve 20](https://documents.blackmagicdesign.com/UserManuals/DaVinci-Resolve-20-Colorist-Guide.pdf), printed pp119–135 (PDF145–161), full local text inspected. Skin work combines balanced exposure, selective corrections and context. The skin-tone indicator is a guide, not a universal target. Lighting, complexion and motion prevent a single optimal recipe. Face masks may require repair when colors resemble surrounding material. This argues against claiming semantic skin from a box/color sample and against forcing identical hues.
2. [FilmLight, Baselight 6 datasheet](https://www.filmlight.ltd.uk/pdf/datasheets/FL-BL-DS-1039-Baselight60.pdf), pp1–4, extracted text inspected. Separates look development from shot grading; describes scene-referred opponent-color processing, color-sector operations, highlight bleaching and smooth zonal operations. Supports separating global look design from local adjustments. Proprietary formulas are not available here.
3. [FilmLight, Baselight v7 datasheet](https://www.filmlight.ltd.uk/pdf/datasheets/FL-BL-DS-1028-Baselightv7.pdf), pp1–2, extracted text inspected. Object mattes, edge refinement and depth are separate tools. Its Segment Anything workflow uses points/rectangles, so it does NOT establish our required fully automatic discovery. It explicitly addresses hard machine-generated matte edges; segmentation alone is not a finished grading mask.
4. [Blackmagic, Resolve19 New Features](https://documents.blackmagicdesign.com/SupportNotes/DaVinci_Resolve_19_New_Features_Guide.pdf?_v=1712905210000), ColorSlice p46, indexed excerpt inspected. Color sectors support density, saturation and hue with brightness-dependent controls. These are color selections, not semantic object recognition. Relevant to the global continuous look layer; no inference that this alone supplies scene intelligence.
5. [Kar & Su, CVPR Workshops2025, Temporal Consistent Semantic Video Color Transfer from Multiple References](https://openaccess.thecvf.com/content/CVPR2025W/CVEU/html/Kar_Temporal_Consistent_Semantic_Video_Color_Transfer_from_Multiple_References_CVPRW_2025_paper.html), abstract and indexed method excerpt inspected; full paper fetch403. Research separates semantic correspondence and temporal stability, uses online training. Not adopted: no local runtime/license/memory/AE random-access validation. Do not transfer its reported quality to our implementation.
6. [MovingColor, authors' project](https://yidong.pro/projects/movingcolor/), project description inspected. Region edit fusion and temporal consistency are explicit problems. No runtime executed; not selected as a dependency.

## Proposed architecture — engineering inference, not an existing algorithm

1. **Explicit input interpretation.** SDR and known Log/gamut conversions remain separate. Analyze normalized input before creative LUT/contrast/temperature. Do not infer Log from low contrast or claim scene radiance from SDR. Preserve known colored illumination rather than neutralizing it automatically.
2. **Whole-frame analysis.** Automatic semantic/material probabilities (skin, hair, clothing, sky, vegetation, water, built surfaces, lights/reflections) plus unknown/background coverage. Probabilities may overlap; confidence must survive into processing. Categories are a proposed acceptance set, not a chosen model's verified capabilities. Instance identity only where needed for temporal coherence; no promise every physical object can be named.
3. **Appearance and light evidence.** Local hue/chroma/luminance distributions, tonal zones, boundary/texture evidence and clipping/noise confidence. Use semantics to constrain appearance analysis. Separate colored surfaces from lighting only when evidence permits; otherwise preserve the observed relationship. No absolute exposure/illumination certainty from one RGB frame.
4. **One versioned creative look.** Global hue/chroma/density/tonal response is continuous across the image. Optional bounded residuals for material and brightness zones share that look; skin is one role. Preset data must describe intended relationships and limits, not arbitrary target RGB values for labels. Tune Natural for neutral behavior, Teal & Orange for first measured prototype, then the rest of the library. Existing baked LUTs remain versioned and unchanged.
5. **Soft fusion.** Residual weights must sum to at most1; uncovered or uncertain areas retain the common look. Preserve detail and avoid luminance seams/halos. Refine mixed boundary pixels; broad coarse support for tiny/blurred objects, finer edges only when supported. Hard label maps are never directly used as grading masks.
6. **Temporal agreement.** Explicit bounded frames, shot-cut rejection, occlusion confidence, matched-object geometry and mask/correction-strength stabilization. No previousFrame. Do not independently recompute unconstrained WB/exposure each frame. Retain short verified bridging; long unsupported gaps revert locally to the ordinary look. Other confidently analyzed objects continue Smart processing. Random seek/reverse/MFR must agree.
7. **Native integration only after proof.** Research models outside plugin; choose one licensed deployable backend after memory/quality measurements on this Mac. Native typed C++ processing and render-safe resources; analysis cache independent of preset. No promise of30fps before measurement.

## Observable acceptance and work sequence

| ID | Requirement and required evidence | Current status |
| --- | --- | --- |
| SC1 | Smart Off exact existing output for all looks; Natural, alpha, Mix0, strength100/300 and old parameter IDs preserved | Baseline exists; candidate NOT RUN |
| SC2 | Automatic useful full-frame region probabilities without points, prompts or manual face boxes; skin includes visible non-face skin | NOT IMPLEMENTED |
| SC3 | Different materials with similar RGB get appropriate bounded treatment; same material under distinct light retains plausible light differences | NOT RUN |
| SC4 | One look across whole scene; skin variation retained; highlights do not inherit artificial saturated paint; no brightness seam from regional mix | NOT RUN |
| SC5 | Small/blurred/moving objects, thin edges, reflections/transparency, mixed lighting, entry/exit, crowd crossings and cuts; no visible hard boundary or mask toggling | User reported baseline FAIL; candidate NOT RUN |
| SC6 | Short recovery remains; complex long gaps use ordinary look without stale masks; reverse/random-order equality and MFR checks | Baseline bounded tests exist; new scene behavior NOT RUN |
| SC7 | Actual multi-frame color/mask review, quantitative motion-compensated difference, color-ramp/gray/gamut checks and native8/16/32 ROI/scale checks on same bytes | NOT RUN |

Next concrete work: choose and run one automatic segmentation candidate against a small diverse licensed/local scene set; measure unknown/false-selection and boundary/motion failures before native integration. Build a standalone whole-frame Teal & Orange prototype with global look plus bounded residuals, compare against ordinary LUT and the old face protection. Do not spend effort calibrating fifteen fixed skin gain tables before proving this path. Preserve existing metadata/Log expansion obligations in PLAN; no restoration/training pipeline expansion implicitly authorized.

## Review of this block

PASS: inspected primary professional sources, restored exact code baseline after preserving the inadequate draft, revised requirements to the full scene and separated source claims from design inference. NOT RUN: new algorithm, model/runtime, skin/object visual quality, motion or AE. This block is research/specification, not a completed Smart feature or release.
