# Face-supported skin protection — review, 2026-10-15

Research candidate only. Acceptance is **not complete**: the inspected sequence contains abrupt protection dropouts, and continuous normal-speed perceptual playback remains **NOT RUN**. No AE installation or production promotion. Exact sources, artifacts and checks are bound in [FACE_SKIN_EVIDENCE.json](FACE_SKIN_EVIDENCE.json).

## Changes and acceptance

Independent YuNet face proposals are confirmed by MediaPipe face landmarks. Semantic skin probabilities are used only inside the confirmed face outline, with inward feathering and eye/lip exclusions. This is face-skin protection, not whole-body protection or recognition of a person's identity. IMAGE-mode inference and explicit radius-two temporal windows preserve random-access processing. Current-frame support bounds the final mask; uncertain current faces cannot inherit a painted region from a neighbor.

Actual source analysis is720×1280, separately decoded from the supplied clip. Masks are area-downsampled to the same360×640 native color trial. Color decode, automatic balance, frozen LUT and native renderer are unchanged. This avoids comparing different grades as if they were segmentation improvements. Input remains explicitly assumed gamma2.4 SDR; the camera profile is not inferred.

| Criterion | Observed result |
|---|---|
| SF01 pinned models/local stateless inference | PASS in this research scope. Three external model hashes, model contracts and license sources recorded. No media upload or model redistribution. |
| SF02 all ten known-error cases inspected | Review coverage PASS. Side-lit face recovery and removal of prior baluster/beige false positives observed. Overall face-coverage quality FAIL: strong profile and frame-edge faces remain missed. |
| SF03 same156-frame comparison, current support, repeated requests | Numeric contracts PASS: exact baseline streams, no mask outside current support, exact model repeats155/0/78. Reliable face coverage FAIL on recorded intervals. |
| SF04 all-frame inspection and motion acceptance | Every frame0–155 inspected in overview and enlarged detail. Continuous25fps perceptual playback NOT RUN. Temporal quality NOT ACCEPTED because protection switches off abruptly. This requirement is not fully closed. |
| SF05 regression tests and integration gate | Geometry/NMS/no-face/current-support/cut tests PASS; existing native and sanitizer suites PASS. Synthetic occlusion tests establish boundary behavior only. Real occluder ground truth and AE integration NOT RUN. |

## Known-error corpus

All seven retained Tears of Steel scenes and three authored controls were inspected in the original/previous-mask/new-mask/weight sheet. This corpus is now a **known-error regression set**, not a blind holdout: candidate choices were informed by these failures. Film credit: Tears of Steel, Blender Foundation, CC BY3.0; original provenance is retained in the scene report.

- `test_face`: the side-lit face is selected; architectural balusters selected by the previous model are excluded.
- `test_rooftop`: the baluster false positives are excluded, but the visible profile face is missed. Zero output here is a failure of coverage.
- `validation_lab`: visible partial/frame-edge face is rejected by conservative geometry checks. Also a coverage failure.
- `train_day`, `validation_people`: confirmed face regions observed; hands/body intentionally have no protection.
- `test_night`, `test_action`, `night_blue`, `neutral_grey`: no spurious face region observed.
- `uniform_beige`: prior small false-positive region removed.

No pixel-accurate ground-truth matte, broad skin-tone acceptance or independent unseen-scene acceptance is claimed.

## Actual visual coverage

All7 `overview-*` sheets and all20 `detail-*` sheets from the final render family were inspected, covering every frame0..155 without gaps. The overview shows each whole frame at180×320 with a green mask overlay. The detail sheets compare source, balance+LUT, spatial protection, temporal protection and both masks; each96×96 source crop is enlarged to192×192 with nearest-neighbor sampling. Crop anchors come from source positions, not model tracking. Head departure at the end is visible in the whole-frame overview.

Additional source/result full-frame sheets inspected: `full-036-041.jpg`, `full-054-059.jpg`, `full-138-143.jpg`, `full-144-149.jpg`. Their stored panels are360×640, but the viewing tool can resize the overall sheet. The other generated full-frame sheets were not separately inspected; all frames are covered by overview/detail. This is not a claim of complete native-scale whole-frame review.

The exact reviewed-file list and hashes are in the evidence ledger. Technical changes to decoder completion/provenance require a repeat render; prior visual findings transfer only when corresponding rendered artifacts are byte-identical. Generation alone does not establish inspection.

## Defects found across the sequence

Frame numbering is zero-based at25fps. Times identify frame starts.

| Frames / seconds | Inspection finding |
|---|---|
| 0–36 /0.00–1.44 | Face-local protection generally continuous; no architectural/clothing selection observed. |
| 37–44 /1.48–1.76 | Mask disappears with small tilted face, changing view and nearby phone/hand. Returns at45. Current temporal method does not bridge this. |
| 55 /2.20 | Single-frame dropout during blur, between protected neighboring frames. |
| 64–130 /2.56–5.20 | Generally continuous face protection in inspected sequence; this is a bounded editorial observation, not calibrated accuracy. |
| 131–132 /5.24–5.28 | Dropout around phone/hand occlusion; returns at133. No occlusion ground truth, so do not label every lost pixel incorrect. |
| 140–141 /5.60–5.64 | Dropout during strong camera motion/blur; returns at142. |
| 145→146 /5.80→5.84 | Face remains visible but blurred at146; face-color protection switches off. Clear adjacent-frame discontinuity in the inspected variants. |
| 147–155 /5.88–6.20 | Face partly exits, then mostly leaves frame/is obscured by hair. Missing masks here are not all false negatives. |

The final candidate has23 zero-mask frames, versus39 in the lower-resolution analysis trial. This count includes face departure and legitimate uncertainty and is **not an accuracy score**. Spatial and temporal protection remain visibly similar in inspected stills; current-support clipping deliberately prevents painting unconfirmed faces, so it cannot fix detection dropouts. No obvious mask trails or background activation were observed in the reviewed sheets, but normal-speed absence of flicker/halos/trails is not certified.

Global adjacent-mask error and raw effect-difference jumps include real movement and exposure changes. They are defect locators, not motion-compensated flicker scores or perceptual PASS evidence. The two comparison MP4s are complete156-frame/6.24-second files and decode successfully; successful decoding is not viewing.

## Decision and remaining work

Retain this measured improvement and the negative examples. Do not integrate into AE yet. The next quality change must address small/profile/partially visible faces and transitions around uncertain detection without transferring skin protection onto an occluder or background. Evaluate it against these same failure intervals, an unseen corpus, explicit cuts and occlusion/reveal cases. Smoothly disabling the entire effect is not evidence of successful face protection.

After that change, repeat the every-frame review and actual continuous25fps playback of the complete comparison. The current tool inspection supplies static images; it did not provide an observed moving sequence. The requested complete perceptual flicker acceptance therefore remains open. Existing SDR/realLog/HDR/Windows/AE/ROI/MFR obligations are retained in STATUS and PLAN.
