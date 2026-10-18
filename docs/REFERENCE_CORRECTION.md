# Reference-frame correction interpolation — 2026-10-18

User request: implement simple averaging of flickering values between reliable reference frames. This is a research color-output stage; it does not change the installed AE effect.

## Contract and implementation

REF-001: interpolate the **added RGB correction**, not source pixels: D(t)=(1-u)D(left)+uD(right), u=(t-left)/(right-left). At midpoint this is the arithmetic mean. Soft support blends this target with the current correction.

REF-002: independent automatically associated person tracks. Reference candidates are local maxima of mean skin-field strength times detector confidence, minimum0.25. This score is a heuristic, not a guarantee of a correct reference. Align added correction using a64x64 face-box coordinate system. At most4frame span, explicit current identity at every frame, no crossing cuts, no extrapolation. Missing/unreliable intervals retain the upstream ordinary/Smart behavior; this stage does not invent detections.

REF-003: minimum of both reference supports and current confirmed face geometry, soft weights and normalized overlapping contributions. Preserve ordinary LUT luminance and original alpha; bound reconstructed correction inside unit gamut where applicable. Nonmodified pixels remain exact. Off files remain exact upstream copies; no source RGB averaging. Real illumination is retained in the current base, but semantic correctness under changing illumination is not established.

Files: `tools/benchmark/reference_correction.py` (math and interval policy), `reference_correction_trial.py` (frozen frame research integration), `tests/reference_correction_tests.py`.

## Checks and observed result

Candidate: b3d7f17 plus preserved working changes. No installation, release or push.

- PASS:5mathematical/policy tests: midpoint, unequal temporal weights, soft boundary, zero support, cuts/long gaps/absent identity/extrapolation, invalid nonfinite support.
- PASS:existing20person-analysis tests; `git diff --check`; scoped static scanner exit0, `/tmp/fstr-reference-correction-20261018.json`. Scanner is not runtime certification.
- PASS:36real research frames,3tracks;26frames received bounded interpolation. Maximum luminance difference6.400346756009262e-8; alpha exact. Source/hash/mask bindings checked by runner and comparison tool.
- FAIL improvement criterion: identical-domain estimated-flow face correction MAE rose from0.0006201716630520033 to0.0008812838940814669 (**+42.10%**). Mask-union correction MAE+33.22%. Mask metric unchanged because interpolation changes color output only.
- Visual snapshot: enlarged frame10 inspected; face/cyborg occlusion still yields irregular support. Static inspection does not certify continuous motion.
- NOT RUN:continuous perceptual acceptance, AE host rendering with this new stage, reliable anchors under diverse light/skin/occlusion. No claim of flicker elimination.

Artifacts outside source: `../reference-correction-2026-10-18/evidence.json`, `../reference-correction-2026-10-18-common-motion.json`, `../reference-correction-2026-10-18-review/Individual-Skin-Comparison.mp4`.

## Separate review / disposition

Reviewed formula, timestamp weighting, person separation, immutable explicit window, zero-support behavior, no-cut/no-long-gap checks, overlap normalization, alpha/luminance preservation and result provenance. Removed inherited reverse-order claim from candidate records: upstream renderer determinism does not certify this new output stage. The native cache bridge and earlier experimental changes remain preserved and unpromoted.

The requested arithmetic is implemented and verified; the chosen automatic reference confidence/alignment is insufficient on this sequence. Keep this stage research-only. Next technical decision is to establish reliable reference validity and motion correspondence before promoting any interpolation to native AE. Do not describe the measured regression as a successful flicker fix.

# Follow-up: scalar strength, current-frame geometry — 2026-10-18

User approved replacing spatial reference-color transfer with per-person scalar toning strength. Implemented in `stable_strengths` and `strength_correction_trial.py`. No reference-frame RGB or mask shape is transferred.

For each automatically associated person and uninterrupted shot/run:
- Measure mean current skin-mask/independent-geometry ratio inside confirmed face support (bounded0..1).
- Select anchors every4frames plus run endpoints; detector confidence must be≥0.8. Anchor value is median of eligible strengths at anchor±1 in the same run.
- Linearly interpolate scalar strength between eligible anchors with span≤4. No cross-cut, absent-identity gap, long interval or extrapolation. Unresolved intervals retain upstream behavior.
- Re-evaluate each current pixel through the native C++ person's fixed raw-sample/LUT-directed reference. Apply the scalar through **current** face geometry and current non-skin material veto. Normalize overlaps; preserve upstream body/nonface correction.
- The reference's toning direction is already held per person by the existing native core. The new temporal stage stabilizes only strength. It does not add a new skin model or resolve erroneous geometry. Small/blurred-face geometry remains the existing upstream policy; this change does not implement a new size classifier.

## Observed validation

- PASS:4new scalar schedule tests (isolated spike removed; gradual change retained; cuts/missing/lowconfidence intervals; invalid inputs),5previous arithmetic tests and20person-analysis tests.
-36frames/3people. Baseline face correction motion MAE0.0006201716630520033 →0.0005550283664238772 (**10.50%lower**); mask-union domain22.83%lower. This is estimated-flow diagnostic evidence, not a perceptual guarantee.
- Ablation with the **same current-frame geometry/native grading**, only replacing interpolated scalar with raw scalar: face MAE0.0006241960043553263 →0.0005550283664238772 (**11.08%lower**), union8.78%lower. This isolates the scalar averaging contribution.
- Full-frame mean absolute added correction0.0007934229749177272 →0.0007987191509326092 (slightly stronger, not a simple global fade of Smart).
- Max luminance error6.129741668718989e-8; alpha unchanged. Native Off remains exact baseline copies. The mask comparison metric references upstream saved masks and does **not** measure the newly composed scalar field.
- Enlarged frame10 inspected; irregular current occlusion support remains. Continuous perceptual acceptance NOT RUN. New algorithm AE integration/host acceptance NOT RUN; installed effect preserved.

Artifacts: `../strength-correction-2026-10-18/evidence.json`, `../strength-correction-2026-10-18-common-motion.json`, `../strength-correction-2026-10-18-ablation.json`, `../strength-correction-2026-10-18-review/Individual-Skin-Comparison.mp4`. Producer snapshot saved beside each trial. Final provenance-hardened repeat: `../strength-correction-2026-10-18-v2` (verify repeat below).

Separate source review: checked per-person separation, cut/run split, local median bounds, scalar interpolation time weighting, current-only geometry/material restriction, overlap convexity, ordinary-luminance/alpha preservation, explicit job parameters and stale-source/LUT/reference validation. Unlike the rejected spatial method, this has a measured improvement on the fixed sequence. This is a research implementation using the native core through CLI, not a claim of a shipped native temporal stage.

Final repeat PASS: provenance-hardened v2 matches the first scalar trial byte-for-byte on36/36outputs;36/36Off files match ordinary exactly; producer snapshot hash valid. Receipt: `../strength-correction-2026-10-18-v2/repeat-verification.json`. Final29Python tests and static scanner exit0; no C++ source changed in this scalar follow-up, so native build/host tests were not rerun for it. Existing native bridge changes remain preserved as separate unaccepted work.
