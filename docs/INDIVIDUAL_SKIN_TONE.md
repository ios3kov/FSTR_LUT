# Individual skin tone — experimental core, 2026-10-17

Requirement SMART-SKIN-TONE-001. No installed AE plugin changed.

## Implemented behavior

`SkinToneLook` accepts explicit straight linear Rec709 samples associated by analysis with one person in one shot. It computes weighted medians of the chosen LUT's change in luminance-normalized RGB, removes residual luminance, and bounds extreme chromatic changes. It never chooses a shared beige/skin target or estimates ethnicity. Per-pixel application retains the ordinary LUT luminance and original per-pixel chromatic variation, modulated by the individual reference, soft skin mask and reference confidence. Common gamut scaling preserves direction; source/output gamut edges and numerical black use continuous fades. No fixed light-skin brightness target. Sparse evidence fades gradually from7to32 usable samples. Clipped/extended samples do not seed the reference; nonfinite samples reject. Alpha is unchanged; unknown/Off/zero mask/extended pixels retain ordinary LUT.

Each reference is tied to its immutable in-process look context; changed LUT/strength or a new context requires rebuilding from cached analysis samples. References are not serialized. This small color operation is separate from mask inference. Caller supplies an explicit shot/window and association; no previousFrame state. References must not be rebuilt independently per frame and presented as stable temporal analysis.

`SceneLook` accepts an optional individual reference and uses it instead of the old fixed skin warmth; pure non-skin fields retain their previous behavior. Mixed fields compose sequentially: the individual skin result influences subsequent role saturation, confidence and gamut limiting. Its helper builds references on the same context. Existing callers without references retain their old path. The research CLI optionally accepts shot samples for a single person's field; multi-person identity tracking and per-pixel ownership are not implemented by this slice.

## Checks and limits

PASS:11release and11ASan/UBSan test targets. Focused tests cover synthetic dark/medium/light tones and shadows, sample-order invariance, single low-weight outlier, ordinary luminance/alpha, identity/Natural, zero mask/Off/unknown, stale and malformed references, extended range and numerical gamut-edge continuity. Scene integration equals the independent individual operator for skin-only fields and leaves sky unchanged.

A separate native CLI trial rendered three synthetic tone ramps, ordinary LUT and individualized result; PNG inspected at768×320. Tone differences and smooth synthetic shading remain visible. Maximum absolute luminance differences from ordinary LUT are below1.7e-8 in these fixtures. Artifact directory: `work/skin-tone-2026-10-17` beside this checkout; evidence.json binds the earlier fixture-run source digest (subsequent source changes only tightened nonfinite input validation). These are synthetic engineering checks, not real-human aesthetic validation.

Senior-model review identified hard gamut thresholds; replaced with continuous fades, added boundary regressions. It also identified missing look binding and abrupt sparse-reference confidence, both addressed. This review is not host or visual certification.

NOT RUN: automatic stable per-person association with face/body/exposed hands, multi-person ownership/occlusion, diverse real skin tones under colored/split lighting, continuous flicker, AE integration. The operator preserves the illumination in its source; it does not estimate or undo colored light. A frozen reference alone does not solve tracking, mask leakage or changing illumination. Do not promote this prototype over the existing native path before these checks.
