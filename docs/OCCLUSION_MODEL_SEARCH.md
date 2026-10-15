# Alternative occlusion model search — 2026-10-15

Decision: find a different source of face/occluder semantics after both FaceOcc checkpoints failed. Separate availability, input/runtime correctness, semantic quality, temporal/host quality and distribution rights. No threshold retuning to the known examples, no plugin changes or redistribution of weights/user images. Research Methodology applied: primary model cards, publisher code and checkpoint metadata; availability is not quality acceptance.

## Shortlist and decision

| Candidate | Verified source/availability | Fit and current decision |
|---|---|---|
| DeepLabV3+ ResNet101 occlusion-aware | [Publisher weights/config](https://huggingface.co/ototadana/occlusion-aware-face-segmentation), revision58e6bafa6770de8b015497a269a1f6df74d6da59;251MB downloaded and weights-only decoded | First next semantic candidate. Embedded training config explicitly includes NatOcc_hand_sot, NatOcc_object and RandOcc. Two classes background/face. No quality claim until actual inference and paired preservation tests. Publisher card declares CC0; this does not independently clear all training-data/third-party rights. |
| XSeg3 | [Reference consumer model definition](https://github.com/facefusion/facefusion/blob/master/facefusion/face_masker.py); models-3.2.0 ONNX asset70MB downloaded; published CRC matched | Executed32 cases locally, two crop modes. Preserves blurred55 but accepts copied skin-texture occluder. Rejected for current integration path. Reference consumer lists GPL-3.0; local diagnostic only, no bundled weights/code. |
| SegFace | [Official AAAI2025 code](https://github.com/Kartik-3004/SegFace), [publisher weights](https://huggingface.co/kartiknarayan/SegFace); multiple backbones, code/card MIT | Secondary parser candidate. Face-part accuracy and a different backbone are not evidence of generic occluder rejection. No local test this block. |
| S3POT | [Official code](https://github.com/Bh-Johnny/S3SPOT), [paper](https://arxiv.org/abs/2602.00635) | Relevant conceptual alternative: reference generation plus SAM prompting. Public README describes training adapter and inference from local checkpoint; ready adapter download not found in reviewed README/tree. Multiple large components and reference generation make it a research fallback, not a ready Mac/AE solution. No local test; no blanket claim that weights do not exist elsewhere. |
| FOCUS | [Official CVPR2023 repo](https://github.com/unibas-gravis/Occlusion-Robust-MoFA) | Reconstructs3D face and segments outliers; pretrained model advertised in Docker. BFM2017/ArcFace and legacy environment complicate adoption. Not first trial; local runtime/weights rights not verified. |

Excluded classification-only face-occlusion models: they provide a face-level label, not the pixel mask required here. Standard BiSeNet face parsing is not independently demonstrated to recognize arbitrary occluders. Search results named FTNet can refer to unrelated thermal segmentation; did not treat them as the face-parsing paper implementation. RealOcc/RealOcc-Wild from [original dataset project](https://github.com/kennyvoo/face-occlusion-generation) are useful independent labelled evaluation targets; access/terms and actual evaluation remain outstanding.

## Executed XSeg3 trial

ONNX Runtime1.23.2 CPU on this M1Pro,2 inference threads. Exact model SHA25648ccd7e8541e159a5a754ec9e62df2f12065f7df8f9af842c1750342c6533559; CRC32e99b694c matches publisher. Input BGR NHWCfloat[0,1],256square, output single mask. Raw model score used with existing0.5 support gate; consumer cosmetic blur/remap omitted deliberately. Input variants: existing box1.5 crop and YuNet five-point adaptation to DFL whole-face template. Not exact original consumer landmark pipeline. Same frozen32 cases as previous ablations, not unseen benchmark. Isolated runtime only.

Source55 mask mass180.9219 of prior181.6203 in both modes, improved retention over FaceOcc. Texture patch interior mean0.900384 in both modes, preserving nearly all supplied wrong support. Box flat-patch mean0.404096. Aligned flat has no matched landmark proposal and outputs zero; this is failed face availability, not correct occluder recognition. Lab prop region zero while forehead/cheek retain support. Zero inherited131/132 cannot assess this gate. Results expose a quality failure despite better blur behavior.

Visually inspected55, flat and texture boards. No all32 visual claim, no continuous video verdict. No full156 rendering/AE for this failed candidate. Raw diagnostic masks are finite/bounded and cannot create coverage outside supplied support. Timings in evidence are individual CPU crop inference, not full-frame/AE benchmarks.

## DeepLab preparation and remaining boundary

Downloaded checkpoint SHA2560ee938da54fe72f34916ec1456192c81efe40e846c03ad0a78cb460a3420f6a0. Safe weights-only deserialization finds730 state entries, classes(background,face), original MMSeg0.30/MMCV1.7.1 training metadata. Publisher also provides MMSeg1-style inference config. Input RGB normalized by published ImageNet mean/std;512 sizing and alignment/padding must be preserved. The original saved test config includes multi-scale/flip evaluation, whereas publisher inference example is single-pass: do not equate their scores.

Next acceptance: run unmodified reference implementation locally or establish independently verified export parity; then fixed counterexamples plus visible-face retention, followed by independent real occluders and full sequence. No replacing runtime operators with dummy implementations, no silent ignored weight keys. Download/metadata parsing is not inference success.

Exact private source/config/model/logs/results: `work/occlusion-model-search-2026-10-15`. [Evidence](OCCLUSION_MODEL_SEARCH_EVIDENCE.json). Existing semantic and continuous-playback gates remain open.

Runtime probe: isolated MMSeg1.2.2/MMCV-lite2.1.0/MMEngine0.10.7 reaches official imports but fails because compiled `mmcv._ext` is absent on this setup. Ordinary missing Python dependencies were installed privately and retried; final failure is the native extension, not a model error. No stubs, operator substitutions or fake successful inference. Next step is a compatible CPU build/export environment and reference-parity validation. DeepLab execution remains BLOCKED in this setup; semantic quality NOT RUN. Mac incompatibility of the model itself has not been established.
