# Anchor-driven face transfer — research checkpoint2026-10-15

## Authorized scope

User requested conceptual handling of blurred faces: transfer previously confirmed facial geometry instead of requiring a new detailed segmentation on every frame. Implemented an independent Python research prototype; existing installed/native plugin unchanged. This supersedes neither semantic occlusion acceptance nor continuous flicker acceptance. Source-size adaptive mask work remains separately documented.

## Contract

`transfer_blurred_face(images, anchors, center, cuts, current_evidence, occlusion)` accepts3..5explicit float32 opaque sRGB frames and n−1cut flags. At most two interior missing-anchor frames; nearest single confirmed anchor on each side, within radius2. Caller must supply raw independent confirmed masks, not recovered outputs; the API cannot prove their provenance. Missing/competing anchors, cuts, long gaps or unreliable geometry abstain. No globals or previous-frame state.

Geometry: up to100local features within each anchor box; bidirectional LK checks, minimum6tracks, deterministic robust similarity fit, final residual<=1.5px, bounded scale/rotation/displacement and feature spread. Both transported masks must agree. Separate dense-flow structural evidence is checked against the inverse similarity motion within1..2analysis pixels, so a structural match from another source region cannot authorize a transferred pixel.

Detail: compare Laplacian energy after aligning source geometry. This is an uncalibrated sharpness heuristic, not physical exposure/blur estimation. Geometry and current evidence govern whether transfer is possible. Relative detail only chooses a continuous mixture of transferred detail and a weak inner ellipse. Both severe blur and uncertain detail favor coarse protection. Full coarse limit is35%; transition mixtures can exceed35%. `detail_uncertain` is diagnostic, not itself a35%limit. The revised route can also recover a sharp detector dropout; it is not restricted to classified blur.

Known anchor exclusions are warped and preserved. Current semantic evidence and explicit foreground exclusion multiply the final result. An all-zero exclusion map means no supplied veto, NOT proof of no occlusion. Fine eye/lip holes may disappear during simplification; known object exclusions must be supplied separately. Automatic reliable foreground/hand exclusion is NOT IMPLEMENTED here. No physical directional motion-blur convolution claim.

## Actual evidence

- PASS9unit scenarios: known2pxtranslation and blur; cuts/missing/competing anchors; current veto/evidence absence; uniform textures; same-color independent texture rejection; deterministic repeat/two missing frames; known anchor exclusion preservation; continuous detail weights; invalid evidence.
- PASS existing15adaptive tests and blur-regression script. Positive synthetic geometry tests supply known semantic evidence; they do not validate a real semantic model.
- Actual frozen C2098 frame54/55/56 with independently confirmed raw endpoint masks and fresh current face-channel inference: final trial-v4 returns nonzero weak protection on55, mass119.534 at720x1280 analysis. Mass is the sum of mask values, not skin recall/percentage. Previous size-adaptive tracking produced zero here.
- Actual12frame slice48..59: final sequence-v3 changes55only; other11raw masks unchanged. Reverse evaluation repeats bit-exact. This uses prior saved raw model masks, fresh current evidence and new tracking; not a new end-to-end model run on every frame, not native rendering.
- Planted flat patch produces full abstention; this also loses true-face protection and cannot be counted as successful localized rejection. Copied skin texture still receives mean0.052873 on the frozen interior ROI. Semantic acceptance remains FAIL.
- Inspected square-proportion comparison54/55/56 and texture board. Face55protection is weak/patchy, not a complete smooth face. Continuous normal-speed output-grade review NOT RUN; no claim that flicker is solved.

## Separate review and retained experiment history

Senior read-only source review identified unchecked final fit, differing transfer/structural geometries, and lost anchor exclusions. Corrected and tested. Final static review found no further blocking defect; noted35%applies only to fully coarse mode. No numerical/runtime claims attributed to reviewer.

Initial trial-v1 restored stronger protection but compared unmatched sharpness sampling. v2aligned sharpness correctly and rejected55; this exposed why hard sharpness classification should not govern geometry. v3removed that hard veto, v4adds continuous detail-confidence blending. Final evidence uses v4 and sequence-v3 only; earlier attempts retained privately. No threshold search on55 to claim general reliability.

Next quality gate: disjoint real blur/occlusion motion with appropriate foreground veto and graded video inspection. The weak patchy55mask is a feasibility result, not product acceptance. Multi-face identity/crossings, large rotations, long occlusions, full preview-resolution parity, runtime/memory speed, AE/Windows remain NOT RUN. Non-Sony metadata and other Log/non-camera input plans remain open.
