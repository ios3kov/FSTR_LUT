# SDR-PREP-001 — automatic per-shot SDR preparation prototype

Authorized after d77d718: build a separate core prototype, compare automatic brightness/contrast/saturation with manual grades on the same shots before AE integration. Color-first rules v11.1.0 remain applicable. No installer, model expansion or change to installed Current9838bb4d38c6e9c9.

## Contract and architecture

Independent C++17 `fstr_sdr_prepare` library, not linked into AE. Caller supplies1..16 explicit anchors from ONE known converted SDR shot. Borrowed typed FrameViews carry dimensions/float stride/count, transfer, Rec709 primaries, CPU memory and alpha convention. Same dimensions required; sampling is a bounded96x54 grid per anchor. Visible sampled RGB must be finite SDR[0,1]; alpha<0.5 excluded from estimation. It does not infer SDR versus Log from pixels. HDR/unknown color space is not guessed. Render validates every processed visible pixel, while estimation validates sampled pixels only. Neutral/amount0 and alpha0 retain original object bytes, including hidden RGB.

Estimation uses sorted luminance quantiles (5/50/95%) from all anchors, with no frame-history state. Constant, mostly transparent, low-key(q95<0.12) or high-key(q05>0.35) input yields identity as ambiguous. Large differences among anchor median log-odds(>0.65) or span(>1.5) yield identity as inconsistent. These are conservative research heuristics, not semantic scene/cut detection. Deliberate lighting or camera motion may trigger fallback. At least16visible samples per anchor required.

Flatness is clamp((5.2-logit(q95)+logit(q05))/2,0,1), with estimator logit bounds1e-6..1-1e-6. Below0.05, keep neutral. Otherwise fit q05/q95 towards linear0.015/0.75 in log-odds space, soften the fit by flatness and bound contrast1..1.8 / brightness bias[-0.45,0.45]. These targets are research heuristics, not standardized SDR reference levels. Brightness is a log-odds bias, NOT physical exposure stops. Optional saturation gain1..1.12 is conservative and reduced for already saturated samples; no white balance/skin-color target or profile normalization inferred.

Apply one immutable result to every frame in the shot. Linear luminance maps through logistic(p+c*(logit(Y)-p)+bias), p=logit(0.18), with exact endpoints and no artificial near-black floor. Scale RGB by mappedY/Y, then use one scalar saturation/chroma contraction towards mappedY to fit the SDR cube. This preserves mapped luminance and linear RGB hue direction, not perceptual hue/gamut equivalence. Only floating boundary residue<=1e-12 is repaired after asserting the analytic bound. Amount linearly blends prepared and source RGB; neutral is exact bypass. LUT/manual operations remain separate, after preparation. No hidden per-frame auto-exposure.

## Acceptance

- Independent gray/odds oracles, monotone black/white endpoints including tiny positive values, dense saturated RGB cube tests of output bounds AND target luminance (hard clipping cannot substitute).
- Already wide-range/ambiguous/mixed anchors fall back; reversal and concurrent rendering reproduce results; errors/cancellation/alpha/premult/stride/explicit profiles validated.
- CLI analyze produces versioned recipe; render consumes it separately, with byte/anchor/frame budgets and explicit input transfer. Release and ASan/UBSan checks.
- Compare unchanged published kitchen/performers/clean-wide SDR sources with prior manual grades. Add explicitly artificial flat control to demonstrate recovery behavior; do not call it untouched camera footage. Render all frames with fixed recipe, inspect results and delivery-size labels/crops; check decoder and pixel integrity separately.
- No promotion into AE or claim of automatic professional grading before artistic evaluation. Existing installed plugin/old projects are preserved.

## Result

PASS:3focused Release and3ASan/UBSan targets (preparation, existing color, existing cinema look),6CLI refusal cases including old v1 recipe rejection. Independent gray/odds, dense13³saturated cube luminance/range, near-black, transfer, premult/hidden-alpha, order/thread, cancellation and ambiguity/error contracts pass. The white-anchor regression fails the first median-based estimator with “Flat white anchor remained gray” and passes v2.

Processed240frames of automatic base and240post-base Teal&Orange65% outputs: kitchen72, performers48, clean-wide48, artificial-flat control72. Every base output is finite, alpha-preserving and strictly bounded0..1. Post-LUT output contains floating residue far below any display code (minimum-1.687e-35); the verification records raw extrema and allows1e-7 tolerance there. Existing LUT/core floats were not clamped or changed; only preview quantization rounds/clips to8bit. Re-requesting frame0 after the last frame is byte-exact for each immutable shot recipe.

Final delivery-size still review: v2 makes artificially flattened whites brighter/darks deeper; real kitchen/outdoor grades are restrained. On already edited outdoor sources the automatic base is close to neutral and lighter than the previously chosen manual darkening. These are scoped visual judgments, not objective superiority over a colorist. Three fully decoded1920×608videos: manual Natural versus automatic base14s; original versus automatic base+Teal&Orange14s; explicitly artificial-flat before/after6s. Labels/footer/aspect/watermark reviewed at delivery size. Continuous visual flicker acceptance remains NOT_RUN.

Exact outputs and receipts: [SDR_PREPARATION_EVIDENCE.json](SDR_PREPARATION_EVIDENCE.json), external `../sdr-preparation-20261018/{manual-vs-auto.mp4,auto-look.mp4,controlled-flat.mp4}`. An initial v1 candidate and a strict-bound verification failure on post-LUT floating residue are retained with their causes rather than hidden.

Installed Current9838bb4d38c6e9c9 signature/file inventory is unchanged. No new AE control/default/parameter ID, model or source-profile change. This automatic grading prototype does NOT restore canceled automatic pixel-encoding selection: explicit sRGB/manual encoding policy remains.

Next integration requirements: deterministic anchor/shot selection (the current consistency guard is not cut detection), source/upstream/algorithm-bound recipe caching, old-project-safe optional UI, and new preparation settings/anchor identity in Smart-analysis invalidation. Smart masks were not used in this prototype comparison. AE/Windows/Linux runtime and a truly untouched flat-camera/paired professional Log target remain unqualified. No installer/push/merge/release.

Visual review rejected the initial median-targeted v1 prototype: artificially flattened white uniforms stayed too gray. Added a failing white/black-anchor regression, changed to robust two-anchor fitting, and versioned recipes as `sdr-shot-v2-prototype`. Initial source/binary/recipes/outputs are preserved externally in `initial-median-prototype/`.
