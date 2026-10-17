# Smart completion block — 2026-10-17

Direct user scope: (1) several people/occlusion/cuts, (2) flicker and soft moving edges, (3) repeatability, (4) new combined Smart in AE, (5) independent RAM/disk analysis caching. The block is **not complete**. Do not substitute the native-cache host test for whole-scene/SAM2 host acceptance.

## Verified changes

- Native Vision observation cache implemented and tested in a separate AE effect. Smart Off skips analysis, On reuses, look/strength do not invalidate, changed upstream pixels create a new key. Two AE processes,13renders at32bpc, persistence snapshots and10release/10sanitizer targets pass. [Contract, resource limits and remaining checks](SMART_ANALYSIS_CACHE.md).
- Repeatability isolated further: two independent CPU/strict/seed0 SAM2 processes on the original16frame arm trial produce29/29bit-identical predictions. Same pinned weights/model; Metal remains non-bit-exact. This establishes a CPU reference on one fixed trial, not the exact offending Metal kernel or universal multi-object determinism.
- New independent corpus:36consecutive12fps frames,3seconds229–232 of Tears of Steel, Blender Foundation/Institute, CC BY3.0. All36frames have3confirmed face observations. Pose observations fluctuate between0and1; face fusion does not depend on detecting all bodies. Already graded/compressed SDR, explicit Rec709-limited→sRGB interpretation; not camera-Log or natural-skin ground truth.
- Automatic seed preparer creates3face-skin object seeds at each endpoint (indices2/33) without manual boxes. Independent SAM2 directions save skin-class union, not person IDs. Both SAM and neighboring flow pass union and per-seed coverage endpoint gates on this case: SAM IoU0.826/0.872, flow0.888/0.895. This does not prove identities are retained under crossing or all limbs are tracked.
- Prototype adds bounded spatial feathering (sigma1.2analysis pixels in this trial) and a2frame smooth envelope into the existing baseline at support edges. Unit controls check lower step gradient, feathering of the direction-agreement gate, unchanged distant background/unsupported endpoints and rejection when one small endpoint seed is lost despite a high union IoU. Parameters are trial-resolution settings, not a production scale-invariance claim.
- Real cut check: previous coarse thresholds missed both cuts in224–236s; retained failed scores. Revised pixel-MAE>.18 plus histogram-TV>.35 finds225.083/234.583s (visually inspected adjacent pairs) and produces no false cuts on156continuous C2098frames. Same-palette spatial cuts remain an explicit known miss; dissolve/flash/camera-motion coverage is not closed. Native bounded-window old path already has independent appearance rejection; revised research thresholds are not a new native shot engine.

## Quality review and remaining defects

The36frame scene+face fusion diagnostic reduces motion-compensated skin-mask MAE0.072325→0.052308, about27.7%; this is the existing bounded stabilizer, not proof that added SAM improves flicker. Flow is an estimated proxy and no ground truth is available.

Native whole-frame and enlarged-face comparisons were exported. Initial frame20 inspection found forehead/jaw bands; separate diagnostic layers traced the abrupt forehead band to the unsmoothed direction-disagreement gate. The gate is now feathered along with geometry, while person/clothing exclusions and full disagreement rejection remain. Final snapshot review shows softer transitions, but internal forehead/jaw structure remains in the skin field. Do **not** mark soft-edge/flicker or professional skin toning accepted from high endpoint IoU or successful renders. Some holes represent intended eyes/mouth exclusion; a robust next step separates outer object geometry from current skin-material evidence rather than tracking skin holes as permanent geometry. Crops follow each frame's detector box and preserve aspect ratio; they help inspect face-relative boundaries but cannot certify global tracking. Continuous perceptual approval remains NOT RUN.

## Requirement state

| Item | State | Next concrete boundary |
|---|---|---|
| Several people / occlusion / cuts | Partial real test | Complete occlusion, entry/exit, identity crossing, same-palette cuts and flashes |
| Flicker / moving soft edges | Not accepted | Diagnose skin bands, inspect full sequence, measure skin-directed color changes independently of geometry |
| Repeatability | CPU reference PASS on original clip | Repeat new multi-object route; diagnose Metal operator only if that backend is retained |
| New whole-scene/SAM2 native AE | Not implemented | Choose native offline-shot analysis backend/job lifecycle; no Python/PyTorch render-time embedding; host invalidation/cancel/cache admission |
| Native RAM/disk cache | Vision slice PASS | Whole-scene tensor cache, product byte budgets, MFR/cancellation/cross-process stress |

The original user effect remains untouched. `FSTR-LUT Cache Test` is the separately installed tested native-cache variant. Owned runner plugins are archived. Existing v3/metadata/Log/Windows and historical obligations are retained in PLAN; this experiment does not close them.
