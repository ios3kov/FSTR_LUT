# SegFace: face-scale diagnostic, 2026-10-15

## Scope and acceptance

User requested different face scales and shot sizes. Evaluate frozen52cases:32previous diagnostic cases (7C2098 frames,2planted patches,23scene images), plus20synthetic scale cases (4sources × heights24/48/96/192/320px on720-square canvas). Compare whole-image inference with fixed1.5face-box crop. Keep true-face preservation and skin-colored-occluder rejection separate. An empty mask cannot pass both. Required continuous motion assessment remains open; no AE promotion.

This is an oracle-box segmentation ablation: boxes are transformed from prior detections, not redetected. Retained-support fraction measures agreement with the prior mask, not ground-truth skin recall. Whole-image face heights become approximately17–228px at model input512. Crops normalize the face size but cannot recover lost detail. Scaling changes context/padding as well as size. Frame54/55 faces start around40/44px; their enlarged variants are not genuinely detailed close-ups. Existing scene images separately include real larger faces.

## Exact model contract

Private SegFace Swin-Base CelebA512 weights; HF revision5e093b03c0523f7f32a9845bbbc75ecb027c8bee, file swinb_celeba_512/model_299.pt. SHA256320b1c167191804913323b229a5256b1fc71f1987161fc54dc440e59af45dae8. Safe weights_only checkpoint load and strict state match. Local source archive and scripts are hashed in SEGFACE_SCALE_EVIDENCE.json. Sole model-source change replaces initial pretrained-backbone download with weights=None; full checkpoint replaces backbone too. No forward changes. Serial CPU2threads, PyTorch2.8, torchvision0.23; no concurrency claims.

RGB uint8/PIL bicubic512, ToTensor and ImageNet normalization match inspected upstream.19class argmax selects skin2 and nose10; neck/eyes/lips excluded. Save true softmax skin+nose probabilities separately in raw-NNN artifacts. Coverage maps are resized categorical masks, not model confidence. Gate uses coverage>=0.5, intersects prior support. Crop uses direct512 affine and identical inverse transform.

## Repairs and separate review

Independent read-only senior review checked channel/class/preparation/affine contracts and identified misleading evidence names and inconsistent0.5 boundary. Final probe-v2 uses coverage/gate names and consistent>=0.5. Initial v1 stopped on OpenCV area-resize support maximum1.000000238418579. Final v2 asserts finite values and tolerance1e-6 before clipping rounding only; larger errors still fail. Initial artifacts retained, final results use trial-v2. Source/input images and masks stay private.

## Results

See generated table and exact report below. SegFace is not accepted for integration: inspected blur55 loses most protection in crop mode, while copied skin texture remains admitted. Whole/crop neither establishes robust semantic rejection. Still inspections are not a continuous flicker verdict.

Next useful experiment must preserve this scale matrix and paired true-face/occluder controls. A multiscale union can retain false positives; an intersection can erase faces. Neither is a justified fix from these results alone. Metadata and non-Sony/non-camera input plans remain recorded separately; no implementation claim.

## Measured scale results

Percentage of prior support retained; not accuracy.

| Source | Face height(px) | Whole frame | Face crop |
|---|---:|---:|---:|
| scale_frame54 | 24 | 100.0% | 67.1% |
| scale_frame54 | 48 | 23.9% | 90.9% |
| scale_frame54 | 96 | 100.0% | 92.9% |
| scale_frame54 | 192 | 94.6% | 92.9% |
| scale_frame54 | 320 | 92.7% | 92.9% |
| scale_frame55 | 24 | 100.0% | 0.0% |
| scale_frame55 | 48 | 0.5% | 0.0% |
| scale_frame55 | 96 | 0.0% | 2.2% |
| scale_frame55 | 192 | 79.4% | 1.5% |
| scale_frame55 | 320 | 97.3% | 3.6% |
| scale_test_face_first | 24 | 0.0% | 1.8% |
| scale_test_face_first | 48 | 0.0% | 93.3% |
| scale_test_face_first | 96 | 0.0% | 95.1% |
| scale_test_face_first | 192 | 98.8% | 94.6% |
| scale_test_face_first | 320 | 95.4% | 94.7% |
| scale_validation_lab_last | 24 | 0.0% | 32.9% |
| scale_validation_lab_last | 48 | 69.4% | 29.0% |
| scale_validation_lab_last | 96 | 22.0% | 98.2% |
| scale_validation_lab_last | 192 | 97.1% | 98.2% |
| scale_validation_lab_last | 320 | 97.3% | 97.8% |

PASS:52cases/104bounded final masks; strict load/input oracle; first42scalar masses exactly reproduce v1; five installed bundle files unchanged. FAIL: inspected face-scale preservation and semantic occlusion. NOT RUN: detector reacquisition at each scale, continuous motion acceptance, AE integration. Scanner passed unchanged repository code only; no model-quality inference.

Final review: inspected selected enlarged outputs from the52case diagnostic (blur55, texture, lab prop and3five-scale boards). Clear-face crop helps at48px and above on test_face_first;24px face is mostly omitted. Blur55 crop is0–3.6% across scales; whole-frame output is nonmonotonic. Lab model still includes prop at larger scales. These are counterexamples, not a general minimum face-size specification. No new production code or installed bytes.
