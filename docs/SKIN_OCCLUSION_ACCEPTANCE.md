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

QuickTime successfully opened exact comparison and playback advanced; tools provide sampled screenshots, not a continuous visual stream. User initially reported: "трудно сказать. лицо очень маленькое. вроде не видно". This is preliminary, not PASS. Created and started a5x face-crop comparison, three full25fps repeats (468frames,18.72s). No artificial sharpness/detail enhancement; labels explicitly state enlarged source resolution. User follow-up requested for perceptual review. A player start, screenshots and full decoder pass do not close that check.

Known numerical continuity failures persist: mask disappears at40–41,131–132,140–141 and146. These do not by themselves quantify perceptual flicker, but cannot be removed from acceptance. Continuous visual verdict is pending explicit evaluation; full AE runtime NOT RUN.

## Final decision and next boundary

Temporal-recovery regression: improved with measured losses. General skin-colored occlusion: FAIL. Continuous visual review: pending human closeup assessment; never infer acceptance from silence. AE integration withheld due semantic failure irrespective of playback response.

Next defensible implementation is an independent candidate for semantic occluder rejection, comparing stationary/longer-than-window/co-moving/entering/leaving objects and genuine face turns/blur. Do not expand the current correspondence heuristic into a universal face-mask veto or continue tuning solely to frame55. Existing MVP/OC/BR requirements remain open.

Exact evidence: [SKIN_OCCLUSION_ACCEPTANCE_EVIDENCE.json](SKIN_OCCLUSION_ACCEPTANCE_EVIDENCE.json). Private outputs `outputs/FSTR-LUT-Structure-Recovery-v2`; private trials `work/smart-acceptance-2026-10-15`.
