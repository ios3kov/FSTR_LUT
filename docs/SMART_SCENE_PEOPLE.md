# Scene + people Smart grading — combined research path

2026-10-15, baseline060b879. User requested combining scene recognition with earlier automatic person/face detection, and detail levels based on source-pixel size. This extends the standalone scene experiment; the installed AE plugins are unchanged.

## Acceptance for this block

- Load a candidate with explicit model provenance, known preprocessing and complete weights. Compare on identical inputs/parts/native LUT, distinguishing declared licensing from quality.
- Restore useful automatic face evidence on the known `test_face` failure without manual boxes or whole-body-as-skin classification. Preserve ordinary output outside evidence and exact Smart Off.
- Use source-pixel size independently for head and figure, with continuous coarse/medium/fine mixtures; blur/insufficient detail reduces detail. Never infer source size from AE preview height.
- Keep every role finite, nonnegative and total mass<=1. Explicit current-frame observations and bounded temporal windows only. Long unsupported gaps retain ordinary LUT; no recurrent history.
- Compare the entire156frame sequence and representative independent scenes. Automated motion metrics do not certify invisible flicker. Do not promote to AE before semantic and visual acceptance.

## Implemented combination

`scene_analysis.py` now supports two explicit local safetensors architectures: SegFormer and UperNet. Required model/config/processor hashes and strict loading-info checks reject missing/incompatible weights. HWC is explicit, including1/3pixel-height inputs. No remote code. Model card for pinned `openmmlab/upernet-convnext-tiny` declares MIT; Hugging Face authored the card. This does not license all datasets/dependencies or qualify the complete product for distribution.

`scene_people_probe.py` reuses the verified YuNet + MediaPipe face mesh + cropped skin analysis from earlier work, and the existing pose model's independent person segmentation. Whole-frame person parts remain skin/hair/clothes evidence, not a body-wide skin label. All inference is local, independent IMAGE calls. Pose cap4 and face cap16 remain limits, not crowd coverage.

`scene_people.py` combines these observations with the scene field. Person evidence can restore hair/clothing/skin where the scene model misses a person. Independently confirmed face skin can restore the head without requiring the scene model to agree. Strong two-scale head proposals without mesh may contribute a weak coarse head spot; explicit mesh disagreement does not. A coarse spot can include eyes/hair/mixed pixels: it is approximate head-color treatment, not pixel-perfect skin recognition. It is capped at.35 before confidence/size attenuation and uses the skin-directed creative residual.

Overlap spends available probability mass, never sums full-strength corrections. Other scene roles are reduced only where needed; unknown space keeps the ordinary look. Fusion precedes the existing bounded temporal smoothing. No global hue selection, no original-skin-hue restoration, no independent framewise WB.

## Size and detail policy

Starting research bands, not validated universal thresholds:

| Region | Coarse activation | Medium transition | Fine transition |
| --- | --- | --- | --- |
| Head |24–32source px |48–80source px |128–192source px |
| Figure |64–128source px |256–384source px |640–896source px |

Figure extent uses probability-weighted vertical moments, not a binary.5 box. It equals rectangle height for a uniform rectangular region and varies with soft evidence. Face proposal filtering accepts small analysis-pixel boxes in this opt-in path; mesh is skipped below64source px. Default behavior of older face tools retains its10analysis-pixel filter.

Details and single-scale mesh confirmation enter smoothly over64–80source px, including the transition from skipped mesh to confirmed mesh. A texture/detail heuristic reduces fine masks under blur; it is not a calibrated physical blur estimator. Insufficient face skin detail also reduces detail. Figure coarse masks smooth **existing material evidence** and remain constrained by person support; they do not invent skin for an unclassified body. Head and figure size are separate; no fixed body-to-face proportion. The current temporal pass stabilizes resulting masks, not persistent identities or a per-person size track.

Evaluation source heights: C2098 upright3840(original camera frame); Tears of Steel800(ffprobe1920×800); authored controls240. Analysis inputs are480or200px high. This validates supplied-scale policy, not AE preview-scale parity, which remains NOT RUN.

## Known boundaries

No trained artistic grading model, calibrated semantic confidence, reliable decomposition of lighting/material, universally correct skin masks, crowds guarantee, or native inference export. The C++ bounded TealOrange residual is unchanged. Coefficients remain restrained art direction; stronger look tuning waits for mask/visual acceptance. Full-frame recognition can still mislabel objects. Model selection and integration are separate from the next actual-host trial.

## Review fixes and rejected experiments

A separate senior review identified binary person activation at.5, the upstream10preview-pixel face filter, and the mesh-entry transition at64source px. All have focused regressions. A real fusion run also exposed floating-point negative remaining mass; clamping only the probability budget fixed it. No image-channel clamp was introduced.

Nine original/lifted/darkened recognition views and twelve automatic overlapping crops were tested on three scenes. Neither restored the difficult test_face reliably; these routes were not adopted. Reusing the old independently confirmed face + cropped skin path restored a useful mask on that face. Mask area alone is not recall or segmentation accuracy.

The full scene run predates producer-time raw hashes. Its166owned raw fields were validated and bound in an explicit post-run capture before the separate fusion evaluation. Final evidence retains this limitation. New scene trials emit raw hashes directly. Renderer/LUT, source, source size, part models, people outputs and scene cache are bound to each comparison. Perceptual and host checks remain separate.

## Executed result

PASS:166native-rendered cases (156consecutive C2098 +7licensed scenes +3uniform controls), exact Off and reverse-window equality for every case. Uniform controls contribute zero skin.24scene tests,40existing person tests,15adaptive tests, direct face/turn regressions and9unchanged-native CTests pass. Original installed plugin's5hashes match. Strict model load, label-schema equality and explicit HWC normalization pass on1/3/11pixel-high inputs. Final source/evidence hashes: [ledger](SMART_SCENE_PEOPLE_EVIDENCE.json).

Visual spot inspection: test_face now has useful confirmed face coverage; train_day shows both faces and visible hands; validation_people retains three heads. C2098frame54 has a soft weak head/arm field instead of a hard box. These are inspected cases, not complete semantic accuracy. On frames148–150, motion blur and arm confidence still vary visibly in the mask. The correction itself is restrained; continuous perceptual flicker remains NOT RUN.

Within the combined sequence, mean skin mask motion error .081944→.065915 (19.6%reduction); whole-field .009238→.007842 (15.1%). Added linear RGB residual MAE .0000951globally/.0005203in the skin-selected region. These estimated-flow metrics do not establish invisible flicker, and the skin evaluation region varies with detections.

CPU scene-only probe on this Mac:3.49–4.45seconds/frame with other local work possible; max resident process size2.25GB. Parts/pose/temporal/native/AE excluded. No Fast certification. Full-scene timed wrapper completed rendering but failed reading system timing counters; separate Python resource measurement is retained.

NOT RUN: new native backend/export/AE, crowd crossings, full preview-scale parity, continuous visual approval. FAIL/OPEN: residual semantic ambiguity and arm-mask confidence changes under strong blur. Current candidate remains standalone; no installed replacement, release or publication.
