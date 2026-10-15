# Automatic person/head support — research checkpoint 2026-10-15

## Contract

No manual face selection. Separate per-person associations in explicit 3–5 frame windows; raw confirmed anchors, mutual unique nose/shoulder matches, scene-cut barriers and no recurrent recovered seeds. Global hand exclusions survive the union of people. Ambiguous observations cannot be erased into recoverable gaps. Four-pose research cap is explicit: reaching it reports potentially_truncated; lower counts do not certify complete coverage.

MediaPipe Pose Full supplies head geometry from landmarks and person segmentation; Hands supplies approximate convex-hull exclusions, not pixel-accurate hand or arbitrary-object segmentation. IMAGE mode avoids hidden temporal model state. Missing hand detections are not proof of no occlusion. Models are locally checksum-pinned and private; redistribution rights remain unestablished. Official references: https://developers.google.com/edge/mediapipe/solutions/vision/pose_landmarker and https://developers.google.com/edge/mediapipe/solutions/vision/hand_landmarker .

Current confirmed anchors without two-sided verification are weakened to 35%. Verified anchors receive structural correspondence caps. Neighboring agreeing hand masks can add a current veto. The different confidence routes can cause strength discontinuities; this is not flicker acceptance.

## Verification and separate final review

PASS: 17 focused tests including competing four-person candidates, ambiguous crossings, capacity reporting, global hand veto, partial off-frame hands, soft-veto behavior, cuts and deterministic repeat. Existing 9 anchor and 15 adaptive tests pass. Final static scanner completed without findings in scanned scope. Synthetic contract checks are not real crowd acceptance.

Fresh C2098 frames48–59 at720×1280: one pose confirmed per frame; reverse rendering exact. Fresh model repeats55/48/59 compare metadata and person/skin/hand arrays, not separately all anchor tensors. Analysis alone448–1051ms on this Mac excludes full AE path and is not Fast acceptance.

Frame55 returns weak partial protection, mask sum122.004 (not recall). Final crop inspected: patchy face mask. Current hand detection misses55; temporal veto does not certify complete coverage. Flat patch causes whole-face abstention, losing true-face protection. Skin-texture patch still receives mean0.409315 in the interior ROI: semantic occlusion FAIL. Combined route is not accepted as superior to the previous anchor-only candidate.

Group control validation_people_last: three confirmed face detections but zero poses, hence zero protection. Close-up test_face_first also loses a confirmed face. Lab control has a head proposal without a matching face anchor. Mandatory body evidence is therefore rejected as the universal solution.

Review corrected ambiguity treated as an empty observation, hand veto undone by mask union, off-frame hand loss, shared face anchors and repeated soft-veto attenuation. Final tests and trial-v7 ran after fixes. Source/report/model hashes are in PERSON_HEAD_AUTOMATION_EVIDENCE.json. Earlier trials remain private diagnostic history, not final evidence.

## Required next step

Use independently confirmed face tracks without mandatory body visibility. Body/head tracking supplies additional support during blur or turns. Associate every person separately; ambiguous crossings suppress only affected tracks. Crowd scheduling must honor source-size detail bands, report capacity and never silently present omitted people as protected. Validate real crossings, entry/exit, close-ups without shoulders, hands crossing another face and many small faces.

NOT RUN: real crowd crossings, continuous perceptual flicker, full source-resolution scale equivalence and AE integration/performance. Skin-colored objects remain FAIL. No native/installed change; five installed file hashes unchanged. No upload, push, main modification, merge or release. Broader metadata/Log/color obligations remain open.
