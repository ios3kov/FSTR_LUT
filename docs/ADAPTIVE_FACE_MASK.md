# Adaptive face-mask prototype — 2026-10-15

User authorized size-dependent mask detail and bounded face tracking. Research only, not installed in AE. Existing color/metadata/Log plans and semantic occlusion/flicker acceptance remain open.

## Contract and acceptance

`FaceObservation` supplies analysis-space box, detection confirmation/confidence, medium/detail masks, detail confidence and explicit permission for a simple ellipse. `source_scale_y` converts face height to source pixels, including orientation. Immutable caller snapshots; no identity embeddings, global previous frame, video-mode model state or recursive recovered seeds.

Nominal source-height bands: <24px abstain;24–63 small;64–159 medium;160+large. Continuous activation24–32, simple-to-medium48–80, medium-to-detail128–192. These are initial engineering settings, not measured universal reliability thresholds. Simple inner ellipse max35% before confidence, no eye/lip holes; requires explicit `simple_allowed`, default false. Medium/detail are supplied masks, not fabricated skin recognition. Lower detail confidence can select simple only with explicit permission. Blur-confidence calibration is NOT IMPLEMENTED; tests use declared confidence, not an automatic blur classifier.

Tracking accepts at most5frames, radius2 around requested center. **This API uses n cuts: cuts[i] separates i−1/i; cuts[0] unused**, unlike older n−1 helper API. Mutual unique overlapping proposals associate faces; unconfirmed competing boxes veto temporal association. Flow+photometric+structural checks transfer neighbor masks; missing correspondence preserves current output, rather than treating unknown as zero skin. Current support cannot expand. Neighbor smoothing is conservative mask attenuation, not a full stabilized box/landmark trajectory.

One/two-frame gaps require unique bracketing detections, clear interior, no cut, agreeing warps and explicit current semantic evidence. Recovery is half the common mask, capped by that evidence. Missing evidence disables it. This is correspondence-based recovery, not proof that a hand or skin-colored object is absent. Large motion, crossing people, longer gaps and uncertain proposals abstain.

## Actual verification

- PASS15 focused tests: tiny/small bounds, detail holes, confirmation, continuous size weights, native-height policy, translated target, one/two-frame gaps, both cut directions, competing weak proposals, flat/changed appearance refusal, invalid-flow current preservation, explicit ellipse permission, current evidence cap, exact request-order repeat and invalid inputs.
- PASS existing blur/turn regression scripts, including existing structural correspondence path. No claim that these synthetic tests prove real skin-texture occluder rejection.
- Actual detector+mesh+skin executed on20prior synthetic scale inputs. Fresh proposals replace the prior oracle boxes. Trial uses mesh-based detailed mask and lightly smoothed medium mask, explicit semantic permission for simple mode. Same images, not a new independently labelled corpus. Whole current mask generation runs; temporal stage is separate.
- Inspected three5-scale detail boards: clearer small face gets weak simple protection at48px;24px often abstains. Blur55 fails detection/confirmation at all five scales. Frame54 at320px also loses protection under stricter confidence gating. Lab prop remains admitted at larger sizes. FAIL robust scale/occlusion acceptance; do not loosen confidence just to turn these masks on.
- Actual C2098 frames54/55/56: fresh analysis and3frame tracking executed, repeat bit-exact. Source3840x2160 with90degree rotation → displayed2160x3840; analysis720x1280 → source_scale_y3. Center55 result remains zero: ambiguous/incomplete current evidence does not authorize recovery. This is a failed quality case, not successful blur tracking.
- NOT RUN continuous normal-speed perception, long-video/new-camera/multiple-person crossings, full native preview-resolution parity, native integration/host checks. No AE installation or production code changes.

## Separate final review and next step

Independent read-only senior review found old structural-guard omission, unknown-flow attenuation and ignored weak competitors. Corrected and regression-tested. No new blocker found in final source review; requested explicit semantic-cap/default-ellipse tests added. Code correctness checks do not establish visual acceptance.

Next: improve current-face confirmation on the retained24px/blur/320px failures and evaluate independent real small-face motion/occlusion material. Avoid treating unreliable detail as permission to paint an ellipse over an occluder. Keep this candidate research-only. Existing full continuous visual acceptance remains unresolved; no repeated user preview request issued.

Private diagnostics: ../adaptive-face-2026-10-15; exact hashes/results in ADAPTIVE_FACE_MASK_EVIDENCE.json. New public-source code: tools/benchmark/adaptive_face_mask.py and tests/adaptive_face_mask_tests.py. Existing production behavior untouched.
