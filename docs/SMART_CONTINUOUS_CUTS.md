# SMART-CONTINUOUS-CUTS-001 — continuous Smart comparison

User authorized continuous Smart OFF/ON across cuts and ordinary AE preview after SDR-SHOTS-001. Product baseline6f96e81 / installed42ee1edaaff01753. No new product defaults/models/processing changes are planned; repair only observed defects within this scope. No installer, release or publication.

## Acceptance

- C1: verify the actual installed package inventory/signature and preserve it; use only a nonce-owned project/process/helper.
- C2: export the complete168-frame960x54024fps concatenation in both modes, cuts72/120. Identical unmanaged32bpc/sRGB, Auto Prepare/Detect Scene Changes/Natural ContrastOn, Teal & Orange65. Only Smart differs. Decode every frame; compare OFF with independent core references (maximum one8bit export code), verify dimensions/opaque alpha/no black frames and measurable Smart differences.
- C3: SmartOff→On and selected out-of-order repeat frames agree within one8bit export code. Track inference/cache counters; distinguish retained cache reuse from legitimate bounded-cache eviction. AE rendered-output reuse also satisfies avoiding redundant analysis; zero inference alone does not prove all raw masks remain resident.
- C4: readable full-frame and enlarged-face OFF/ON comparison videos, two identical normal-speed repeats,24fps/14s, no color amplification; fixed crop inside each shot, labels and source attribution. Inspect actual full/cropped stills, including cut boundaries and largest motion-compensated residual candidates. Numeric residuals are diagnostic, not proof of perceived flicker or skin ground truth.
- C5: actually run ordinary AE preview of the plugin composition via UI and verify running/time progression/stopping; rendered queue exports cannot establish preview. Record whether cached and any speed limitation.
- C6: continuous human flicker acceptance stays distinct from sampled still inspection and motion diagnostics. Do not infer acceptance for the new comparison from the earlier user report about other clips.
- C7: save the editable own project; normal own AE exit; archive the exact helper, verify installed product unchanged; separate final review and resumable Git checkpoint.

## Source fidelity

Reuse `../sdr-shot-detection-20261018/sequence/`, exactly168previously verified frames from the official DPReview Nikon Z6 Flat reel. The publisher already adjusted the footage; this is not an untouched camera original, unknown Log inference or a paired RAW grading comparison. Embedded watermark/4K source text is retained; individual host outputs are960x540. Research crops are for visual assessment only; the plugin still discovers people/objects automatically.

## Current evidence

**Installed product42ee1edaaff01753 unchanged; no product-code changes.** All artifacts/logs/scripts and the saved project are in `../smart-continuous-cuts-20261018/`.

| Criterion | Result | Actual evidence and limit |
| --- | --- | --- |
| C1 | PASS | Exact installed signature/inventory before launch and after normal owned exit; nonce/project guards; previous6f96e81 sources retained |
| C2 | PASS scoped |168OFF+168ON exports decoded; all168OFF agree with independent full-float core references within one8bit code; opaque alpha/geometry preserved; no black frames. Each shot has Smart differences above two export codes, avoiding a dither-only assertion. Mean absolute ON/OFF difference is0.351/0.489/0.363code across the three shots, maxima5/6/5codes; this material shows a mild effect, not a dramatic recolor |
| C3 | PASS scoped | Six selected ON repeats and oneOFF repeat agree within one8bit code; model counter156unchanged through toggling/repeats and UI preview. Earlier four isolated cut-adjacent exports also agree with the continuous sequence within onecode. AE output-cache reuse can satisfy this; this does not prove every raw mask remains in the512MiB disk cache |
| C4 | PASS artifact/sample scope | Both336frame24fps14s1920x648 movies fully decode; same normal-speed sequence repeated twice, no color amplification; fixed equal crops on both sides. Full-size/crop samples and three diagnostic pair boards inspected, no observed hard color contour, displaced geometry, missing output or obvious shade jump in those stills. Labels/footers/glyphs fit and preserve source attribution |
| C5 | PASS running/time/stop only | Actual CUA Space preview of SmartON composition at Full/32bpc; complete0..7s green cache bar; Play→Stop square and displayed00:00:00:06 after start, stopped at00:00:01:08(frame32) with triangle restored. Zero new model inferences. Viewer remained200%; unsupported AX/custom controls and menu attempts did not establish a full-fit view or measured playback FPS.24fps in the timeline is nominal, not measured real-time proof |
| C6 | NOT_RUN human continuous perception | Tools inspect snapshots; normal-speed full/cropped videos are supplied for human visual acceptance. Prior acceptance of different clips is not transferred. Motion diagnostics do not close this requirement |
| C7 | PASS | Editable own `host/Smart-Continuous-Cuts.aep` saved. Normal CmdQ of own PID74384, process absence verified, helper archived, installed signature/inventory unchanged. No forcekill, installer or publication |

Total343exports. Actual observed render-queue elapsed: OFF23.575s; ON462.341s (7min42s) for7s of video; selected cached ON repeat jobs138–228ms. This is **slow first-pass analysis**, not a real-time implementation. The ON pass reused some earlier disk analyses and independent core-reference generation overlapped its early part, so these are observed workflow timings, not an isolated or portable performance benchmark. Warm job times can include AE rendered-output reuse and do not measure raw Smart model speed.

The165within-shot transition diagnostics exclude cuts72/120 and warp the Smart-minus-OFF difference with backward optical flow at320x180, rejecting inconsistent/photometrically changed pixels. Largest eligible face-and-surroundings ROI mean residual is0.2062encoded8bit code. Representative worst pairs per shot13→14,115→116,156→157were inspected as full-size crop boards. ROIs are not skin ground truth; flow error, occlusion, illumination and8bit export dither contribute. No perceptual PASS follows from this number.

Deliverables: `Smart-OFF-ON-Cuts.mp4`, `Smart-Faces-Cuts.mp4`; decoded PSDs, independent OFF references, crop/diagnostic boards, native/cache/preview/cleanup receipts. [Evidence manifest](SMART_CONTINUOUS_CUTS_EVIDENCE.json). Next meaningful engineering work, if authorized, is reducing first-pass analysis cost while preserving these fixed visual/reference cases; restoration/model expansion is not part of this verification.
