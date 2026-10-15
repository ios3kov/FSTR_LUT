# Skin-colored occlusion and continuous review — 2026-10-15

User requested closing skin-colored object and continuous flicker checks after af88cb0. Apply existing OC/BR criteria, production-engineering regression workflow and AE Rules11.1 Development gate. No relaxation of acceptance, native integration, installed-plugin replacement or publication. Real frames with synthetic patches are explicitly distinct from real occluder footage.

## Reproduction and correction

The old temporal recovery accepted an unrelated skin-colored textured patch almost fully (seed731 interior mean0.9954). Absolute color difference and forward/backward motion agreement were insufficient. Added local normalized structural correlation after equal Gaussian filtering and motion warping, with a2px inward margin. This only restricts new temporal recovery; it cannot fix false pixels already present in the semantic face mask.

Executed84 paired synthetic cases:21 fixed seeds × control/flat skin-colored patch/unrelated skin-colored texture/green. Final textured maximum interior mean0.03198; flat maximum0.01193; green0. Control outside minimum0.9527. Visible unoccluded outside minimum: textured0.8090,flat0.9176,green0.6177. These nonzero residues and losses remain explicit; this is a bounded regression improvement, not arbitrary occluder recognition or proof of no side effects. Separate unit test rejects the original731 regression and preserves translation/cut/input contracts.

Full156-frame native repeat and23 static scenes executed. Native mask and render bytes are identical to af88cb0's final trial, including partially recovered55. Full comparison media also identical. Numeric background/luma/opaque output and nonchronological repeat checks pass. Installed five bundle files unchanged.

## Deeper semantic counterexample — FAIL

Pinned actual FaceOcc inference repeated on five720x1280 C2098 frames53–57. A planted flat/skin-texture rectangular patch overlays face55. Original current support and source boxes are reused deliberately to isolate semantic gating. The control reproduces prior gate mass1.3415346. Flat and textured patches receive mean gate weights0.7521 and0.8259 in their interior; the mask falsely accepts the object before temporal recovery. Visual comparison inspected. This is a real-image controlled counterexample, not a benchmark of actual moving hands.

`no_major_dropout` leaves it unchanged, and `maximum(base,recovery)` would retain it even if that early return were removed. Applying temporal veto to all accepted masks changes the previous nondecreasing-mask contract and could remove genuine skin during turns/expression/blur. Independent read-only review confirmed this limitation and the cost in outside retention. General skin-colored occlusion remains FAIL; candidate must not be promoted to AE.

Initial private image probe had incorrect timestamps after frame selection and duplicated selected frames; rejected. Final runner resets timestamps with `setpts=N/(25*TB)`, checks exact source identity and frame byte count, and reproduces original control inference. Its separate sRGB-area downsample for recovery differs from the frozen native-baseline preparation; do not equate its101.66 control mass with native99.25.

## Continuous review

QuickTime successfully opened exact comparison and playback advanced; tools provide sampled screenshots, not a continuous visual stream. User initially reported: "трудно сказать. лицо очень маленькое. вроде не видно". This is preliminary, not PASS. Created and started a5x face-crop comparison, three full25fps repeats (468frames,18.72s). No artificial sharpness/detail enhancement; labels explicitly state enlarged source resolution. User subsequently answered the enlarged review: "Пока не могу оценить". No reliable perceptual verdict is available; do not keep asking in this block. A player start, screenshots and full decoder pass do not close that check.

Known numerical continuity failures persist: mask disappears at40–41,131–132,140–141 and146. These do not by themselves quantify perceptual flicker, but cannot be removed from acceptance. Continuous visual verdict remains inconclusive after the user could not assess the enlarged preview; full AE runtime NOT RUN.

## Final decision and next boundary

Temporal-recovery regression: improved with measured losses. General skin-colored occlusion: FAIL. Continuous visual review: no reliable verdict after enlarged preview; never infer acceptance from silence. AE integration withheld due semantic failure irrespective of playback response.

Next defensible implementation is an independent candidate for semantic occluder rejection, comparing stationary/longer-than-window/co-moving/entering/leaving objects and genuine face turns/blur. Do not expand the current correspondence heuristic into a universal face-mask veto or continue tuning solely to frame55. Existing MVP/OC/BR requirements remain open.

Exact evidence: [SKIN_OCCLUSION_ACCEPTANCE_EVIDENCE.json](SKIN_OCCLUSION_ACCEPTANCE_EVIDENCE.json). Private outputs `outputs/FSTR-LUT-Structure-Recovery-v2`; private trials `work/smart-acceptance-2026-10-15`.

## Additional fixed input-contract ablation — rejected

Executed32 cases (seven actual source frames, both planted occluders,23 existing scenes) with unchanged modernized weights and0.5 threshold: current max-of-three-rolls, same crop single orientation, and pinned upstream POS geometry adapted to YuNet five points. Source/template at https://github.com/mertakinstd/FaceOcc/tree/8d86c9a2c1a0094dbc1a1e5467413b49f8936b32/face_align . Scale/translation and inverse mapping passed an independent projected-landmark oracle. This is a YuNet adaptation, not reproduction of original3DDFA landmark generation. Modernized ImageNet normalization retained. SciPy1.16.2 installed only into isolated private research deps to read the MAT template; no runtime/plugin dependency changed.

Single orientation still admits the textured patch (interior0.7521), despite improvement on flat patch0.0439. Aligned output removes both patches by deleting face support: flat case has no matched YuNet proposal, texture case entire mask is zero. On genuine frame56 it retains18.85 mask mass versus183.31 current (~90% loss). This fails visible-face preservation. Neither variant promoted. No further crop/roll/threshold tuning applied.

Inspected both planted cases,frame56,and lab prop/side-lit face/people scene boards. Corrected those scene review boards to preserve aspect ratio; numeric arrays unchanged. Geometry and all32 numeric results are recorded; other generated boards were not claimed visually inspected. Initial private run failed with list/array IoU type mismatch before results; corrected rerun uses explicit array and a fresh output directory.

Private reproducible source and all results: `work/smart-acceptance-2026-10-15/alignment/probe.py` and `alignment/result-v2`. Exact source/template hashes in evidence. This closes the input-preparation hypothesis as unsuitable on tested cases, not the semantic-occlusion requirement. Next candidate must improve the semantic model/path with independent occluder/skin ground truth. Current AE transfer remains blocked by known quality failures.

## Original checkpoint and original normalization — rejected

Downloaded the official original FaceOcc `epoch_16_best.ckpt` from the upstream linked Drive file. SHA256 `01d3c3939c28e47a45acb9a5ea8f8ee460e5ecf046c0caa8404e05915e10901b`. Safe weights-only CPU loading, exact module-prefix removal and strict182-key match passed. Original upstream evaluation at `e75d4a83a696bd7379128319244ef6e5e7885fc8` uses RGB ToTensor [0,1], without the modernized ImageNet normalization. Applied that original contract, preserving the fixed prior32 cases and three geometric variants. No global dependency, native code, installed bundle or previous candidate changed.

This does not reproduce the complete original evaluation: boxes and YuNet-adapted POS remain our research inputs, not3DDFA preparation; inverse probability interpolation and the established gate remain unchanged. Mode `current` in the saved report denotes the existing three-roll geometry with ORIGINAL weights, not the modernized baseline.

Original three-roll path accepts flat and textured planted occluders with interior mean0.90038 and0.54297. Single orientation still accepts texture0.34397. Genuine frame56 mask mass falls from modernized183.3065 to original94.5404; single75.3361; adaptedPOS0. Original55 mass22.9203 remains very partial. Thus original weights do not solve semantic rejection or face preservation. Lab prop remains rejected, which does not override the two counterexamples. No thresholds retuned, no failed candidate native rendering/integration.

All32 cases/96 masks numerically finite and bounded; visually inspected both planted patches and genuine56 closeups. Other boards not claimed inspected. Inherited zero support131/132 does not test this gate. Full156 repeat, native render and AE for this rejected candidate NOT RUN; continuous perceptual acceptance remains unresolved as above. Exact evidence: [ORIGINAL_FACEOCC_EVIDENCE.json](ORIGINAL_FACEOCC_EVIDENCE.json). Reproducible local runner, weights, original upstream files and outputs: `work/smart-original-faceocc-2026-10-15` outside Git. No weights or user images redistributed.

### Raw six-channel diagnostic

Following independent read-only review, executed local MediaPipe raw six-channel inference on original54/55/56 and the same two planted55 counterexamples. Used the proposal nearest fixed patch center and unchanged2.6 crop; no new thresholds. Saved all six maps. Flat patch mean face-skin0.0425 / others0.7169, whereas texture patch face-skin0.9060 versus source55 region0.9136. All interior texture pixels choose face-skin by argmax. Face-only evidence can reject the flat patch here but does not separate copied skin texture. This remains a planted counterexample, not proof of performance on real hands. Same-image-coordinate region on54/56 is not tracked skin ground truth, so its changes cannot be called temporal semantic failure.

The FaceOcc planted tests deliberately use frozen prior support to isolate that gate; they do not show how a fully recomputed upstream semantic mask handles the flat patch. Keep this distinction in any product claim. Initial channel run failed an unjustified single-proposal assertion; corrected fixed geometric target selection and reran all five cases to fresh output. Numeric diagnostic only; no continuous visual acceptance.

Separate final review: original symptom persists in both original checkpoint and face-only channel; local semantic candidates rejected, no accepted replacement. Further completion needs a genuinely different occlusion model and independently labelled moving-hand/object and visible-face cases, not more threshold/roll tuning on these examples. Full requested Smart Skin completion remains blocked by quality, not by a request for user approval. Existing color/LUT plugin preserved.
