# Bounded native region interpolation — 2026-10-15

User requested interpolation between confidently detected faces, retaining a simple soft color object through blur. Implemented in C++, called by SmartFX; not installed or validated inside AE.

## Contract

IR01: explicit complete window offsets -2..2, raw detected endpoints on both sides, at most two missing frames; never recursively use recovered masks or hidden previousFrame. Boundaries and incomplete windows do not extrapolate.

IR02: mutual unique scale/position association; retain conservative existing match distance. Predict normalized box by time interpolation, then search up to one quarter of its width/height around that position. Every intervening frame must match both raw endpoints using mean RGB in nine area-averaged cells (maximum of both mean errors<=0.08). This intentionally compares broad color objects rather than facial texture. Global adjacent-frame signature error>0.10 blocks transfer; this is a heuristic and cannot guarantee detection of same-colored scene cuts.

IR03: any detection intersecting the predicted box at an intermediate sample rejects that pair; local search cannot evade it. Selected and final boxes cannot overlap occupied regions. Ambiguous endpoint matching, local appearance replacement, missing samples and longer gaps reject recovery. Recovered regions have detail0 and no holes, using the existing soft mask and strength.

IR04: Off behavior unchanged. On SmartFX checks out up to four neighbors, preserving full-input analysis and output ROI. All successfully acquired worlds checked in on failure/cancel. Legacy current-frame render does not gain temporal interpolation. No arbitrary-object detector or complete body/head fallback is claimed.

## Actual results

Native replay of prior actual Vision observations and exact540x960 analysis inputs across156frames restores only50 and116. Detection/nonempty coverage78→80frames; not an accuracy score. Frame-order permutations produce identical recovered geometry. No claim of fixing the long gaps around54–56 or all blurred faces.

Initial point-sampled texture comparison restoredzero. Investigation showed motion/blur changed texture samples and a sparse global change gate falsely rejected these same-scene pairs. Replaced both with spatial color averages and bounded local search; did not increase acceptance thresholds. This selection used the supplied clip, so unseen-scene validation remains required.

Real module callback fixture executes actual Vision on frame50 and four neighbors: recovery changes504pixels at540x960, preserves alpha, balances checkout cleanup. Real frame48 Smart On still changes563pixels and Off restores original LUT bytes exactly. These are owned SDK callback tests, not actual AE runtime.

Six-frame independent native color rendering at360x640 (49/50/51/115/116/117) with Teal & Orange: only missing50/116change (249/277pixels); four anchor outputs are exact; all finite, alpha1. Inspected enlarged color comparisons and masks: restored areas lie over faces and edges are soft. Static inspection does not establish continuous perceptual flicker acceptance.

Eight Release CTest targets and eight ASan/UBSan targets PASS. Added stationary/moving midpoint, one/two-frame gaps, long-gap rejection, cuts, local occlusion, ambiguity, intermediate competitor, missing/duplicate frame tests. Extended callback failure to checkoutID4. Senior review corrected raw-error threshold versus search tie penalty and intermediate competitor bypass. No critical ownership finding in reviewed scope.

## Remaining

NOT RUN: continuous perceptual playback, actual AE UI/ROI/MFR/save-reopen/Undo and source-time behavior, unseen crowd crossings, similar-color scene cuts. Full Smart quality remains incomplete because the native detector has longer missing intervals, including the originally reported54–56. Existing installed plugin preserved. No push, merge, release or installed replacement.

Next stage remains broader head evidence/longer dropout handling and isolated host validation. Do not infer readiness from the two restored frames. Exact source/artifact hashes and logs: NATIVE_REGION_INTERPOLATION_EVIDENCE.json.
