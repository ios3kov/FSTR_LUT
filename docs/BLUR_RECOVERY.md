# Motion-supported blur recovery — research, 2026-10-15

User authorized motion-aware recovery and directional mask softening after occlusion checkpoint abb8a45. Apply production-engineering bug/regression workflow, AE Rules11.1 Development scope. Native plugin, models, looks and installed bytes remain unchanged. Existing MVP/TR/OC criteria remain open.

## Observable acceptance

- BR01: restore meaningful current-face protection on known blurred frame55 without expanding prior current face support. Compare masks AND native colors at frame55 and neighbors54/56.
- BR02: deterministic nonrecursive bounded neighbors, no cross-cut or one-sided recovery. Invalid input fails. Original cached observations only, never candidate outputs as seeds.
- BR03: decline recovery without relative blur, strong two-sided support and checked motion/photometric correspondence. Test translation and a distinct-color transient occluder; preserve23 existing static scenes. These checks cannot establish arbitrary same-color occluder rejection.
- BR04: directional softening is inward, capped and conditional. Its radius is a heuristic from motion and relative sharpness, not an estimate of exposure or physical blur length.
- BR05: actual156-frame run, exact native/background checks, inspect changed frames and adjacent context, source/evidence hashes and separate final review. Continuous normal-speed perceptual review and full-frame review remain required for acceptance; still sheets/decoding alone cannot close flicker.

## Candidate contract

The routine consumes explicit images, prior support, original visible-face masks, center index and cuts. Radius2 validation; only immediate neighbors can seed. Require current mask mass below70% of support, each neighboring mask at least80%, and current masked Laplacian energy below75% of both neighbors. Both direct checked warps must agree (overlap/union >=.45). Forward-backward flow, photometric difference and local texture reject uncertain pixels. Inward directional feather radius <=2px; output is between original gate and current support. Thresholds are experimental, not calibrated probabilities. These conditions do not constitute a semantic hand/phone detector.

Frozen previous inference is reused with input/report hashes, paired raw/npy exact agreement and a2e-6 tolerance for cross-NumPy floating-point reductions. Model inference is not repeated; the experiment isolates temporal behavior. Actual native rendering is repeated with identical settings.

## Final review / results

Final v2: strict float32 RGB/masks and sides8..2048. Initial diagnostic rejected cross-NumPy exact float-sum comparison; paired cached raw/npy arrays were bit-identical and reduction differences were about1e-5. Corrected validation uses declared reduction tolerance plus exact paired-array check; no inferred new model accuracy.

BR01 PASS for the known partial-recovery symptom, not complete face segmentation: frame55 prior support mass181.6203; visible-face gate1.3415; final99.2455 (54.64% of prior support, not percentage of face pixels or an accuracy score). Local flow approximately(1.03,-10.78)px/frame and inward feather radius0.776px. Warped-mask agreement0.740. Masks and color inspected on frames48–59 detail sheet and native full-frame54/55/56. Face support is partial/irregular; general visual acceptance remains open.

BR02 PASS: tests for deterministic repeat, cuts, weak seeds, invalid masks/dtype and translation; actual nonchronological repeats155/55/0/130/37. Neighbor values remain original observations. BR03 PASS only in exercised scope: current support preserved; distinct-color synthetic occluder interior stays zero while surrounding face support recovers. Sharp controls decline recovery. All23 static scene gates are bit-exact, preserving prior prop rejection; repeated stills do not prove temporal hand/phone robustness. Similar-color occlusion and multiple-face motion remain NOT RUN.

BR04 PASS within numeric contract; inward-only support and directional feather exercised on translated fixture and actual55. No physical exposure/blur estimation claim. BR05 partial: actual156-frame native render, finite opaque output and background/luma checks pass. Exactly55 changes; other155 masks AND native renders are bit-identical to previous candidate. Final v2 media are byte-identical to visually inspected v1 media. MP4 fully decodes156 frames at25fps/6.24s,1080x680 comparison layout. Full continuous perceptual playback and whole-sequence native-scale review NOT RUN; user's flicker acceptance is not closed. Five installed bundle hashes remain unchanged.

Remaining: inherited face misses/profile errors and partial masks, unseen dynamic occluders, continuous flicker/host checks. Candidate stays outside AE. No model/host speed or release claim. Next useful validation requires real moving skin-colored occluders and continuous playback; do not tune more thresholds solely to frame55.

Exact report: [BLUR_RECOVERY_EVIDENCE.json](BLUR_RECOVERY_EVIDENCE.json). Private artifacts: `outputs/FSTR-LUT-Blur-Recovery-v2`, `work/smart-blur-2026-10-15/trial-v2`. No footage/weights committed.
