# Color follow-up — 2026-10-17

## Current decision — SRGB-DEFAULT-001, 2026-10-18

Direct user instruction supersedes AE-AUTO-001: **sRGB by default, editable manually; no Auto mode at all**. This is not a guessed automatic encoding. New effects start at existing popup value2(sRGB); diskID9, values1–4, Advanced placement and rendering meanings stay unchanged. Existing projects must keep stored choices, including legacy unspecified1. Retain that legacy slot/error behavior to avoid reinterpreting saved pixels. Do not implement a main-thread profile bridge, converter stack or additional Auto UI.

Acceptance: fresh effect renders a non-neutral grade without selecting encoding; manual Linear3/Gamma2.4value4 still work; old untouched default1and explicit2/3/4survive loading and save/reopen; the old/new outputs for matching explicit settings agree within the existing host export tolerance. Model analysis/cache remains independent of this UI default change; no Smart algorithm change is in scope. The earlier diagnostics below remain historical evidence for the canceled Auto route.

## SRGB-DEFAULT-001 — actual host acceptance

PASS on2026-10-18, AE25.6x101/macOS arm64. The old installed Current build05dd70f4d7a1b422 created an AEP with untouched legacy default1 plus explicit2/3/4; new signed build d774807231370f48 opened it, rendered all4, added3new effects (asserting initial2 before any changes), rendered sRGB/Linear/Gamma2.4, saved and reopened. All7stored choices persist. Four old/new legacy PSD exports are byte-value equal after decoding; new/manual equivalent pairs differ by at most one8bit code. Linear versus sRGB differs by64codes, proving manual selection affects output. This is a64×64uniform32bpc source/8bit Photoshop export test, not a new Smart visual approval.

14Release and14ASan/UBSan targets pass, including an independent IEC sRGB expected value for fresh non-neutral rendering. Final source review confirms only default1→2 affects the product. All host source files match the root except trial registration identity. Old Current bundle and both test helpers are archived; only the hash-inventoried owned Current trial was updated. Installed inventory and strict/deep signature verification pass after the test process exits. Other installed effects are preserved. No release/push/merge.

Evidence index: [SRGB_DEFAULT_EVIDENCE.json](SRGB_DEFAULT_EVIDENCE.json). External reproducible artifacts: `../srgb-default-ae-20261018/{baseline,candidate,verification.json}`; script `tools/ae_host_runner/srgb_default_acceptance.jsx.in`. Continuous Smart quality, wider footage and workflow checks remain open under AUTOMATIC_SMART.md.

## Historical Auto investigation

All entries below describe the canceled route and their original revisions. Their future-tense Auto steps are historical, not current requirements.

## AE-AUTO-001 — superseded, history

Requested: Auto by default for new effects; manual encoding inside collapsed Advanced. Preserve old disk ID9 and popup values1–4, old projects and render determinism. Unknown/unsupported AE encoding must not silently become sRGB. Accept only when project working-space and Linearize changes, MFR, save/reopen and fresh aerender resolve current input encoding correctly.

Implemented slice: existing encoding selector is inside collapsed Advanced. Topic IDs14/15 are new; old persistent IDs unchanged. Renderer excludes group markers from checkout and balances acquired value parameters on failure. Auto is **not implemented**: SDK ColorSettings is AEGP/main-thread; sequence setup and render may run on workers. A cached main-thread snapshot alone has no established freshness guarantee after project settings changes or in fresh aerender. No guessed default or renamed manual choice shipped. Host migration/UI still NOT RUN.

## SMART-SKIN-TONE-001

User requires adaptation to the original skin tone of each person, including exposed skin beyond faces. Skin should follow the selected look while preserving differences between people, skin texture and illumination. Do not force a common target hue, brighten dark skin to a common target, or infer ethnicity. Existing shared color formulas do not establish this feature.

Planned processing: associate reliable skin evidence with each person; estimate robust chromatic/luminance distributions per shot from explicit frame context; use bounded preset-conditioned residuals per person, modulated by light/shadow and confidence. Reject highlights/clipping and uncertain occlusion from reference estimates. Small/blurred people retain coarse soft fields. Uncertain identity must not exchange reference statistics between people. Long unreliable gaps fade to ordinary grading. No hidden previous-frame state; analysis/cache key includes algorithm/settings/reference inputs, excludes look/strength when those affect color only.

Acceptance: mixed light/medium/dark tones within one shot, different illumination of the same person, visible arms/hands, crossings/occlusion, approaching/receding and cuts. Verify no forced tone convergence, no bleaching of dark skin, no false warm correction under colored light; compare motion stability against source and ordinary LUT. Report numerical checks separately from visual acceptance. Independent-core implementation is now available; automatic person association, representative diverse-tone footage and native AE integration remain NOT RUN. See INDIVIDUAL_SKIN_TONE.md.

## Verification of Advanced slice

PASS: arm64 native build and all10 `build-smart-native` CTest targets; all10 `build-native-sanitize` ASan/UBSan targets. Callback fixture rejects group checkouts and injects failures both after first marker and after second marker, confirming value-only balanced cleanup. Persistent IDs, four legacy choices and old default verified by schema fixture. Separate source review and `git diff --check` passed. Production-engineering code scanner reports no findings in scanned scope (619 text files); runtime certification not assessed. Actual AE UI, old-project migration and Auto: NOT RUN / not implemented. Installed effects unchanged.

## Additional SDK check — 2026-10-18

TargetSDK25.6 still exposes project working-space/profile through AEGP_ColorSettingsSuite6, version7; no new thread-safe render-world color-profile getter was found in AE_EffectSuites.h/AE_EffectCBSuites.h. [Adobe SDK threading contract](https://ae-plugins.docsforadobe.dev/aegps/implementation/#threading) says AEGP functions must be called on the main thread. [PF_ColorParamSuite1](https://ae-plugins.docsforadobe.dev/effect-details/parameters-floating-point-values/) exposes HDR parameter precision, not a documented project-space resolver. A hidden swatch/contrast heuristic is not an authoritative Auto implementation. AE27UXP Project APIs found during current research do not establish AE25.6 native worker compatibility. Auto is still required/open; manual selector remains in Advanced. No unsafe thread call or guessed sRGB default added in the automatic-analysis work.

## AE-AUTO-SWATCH-001 actual-host result — 2026-10-18

The separate identity diagnostic in `tools/ae_color_probe/` tests six hidden float color parameters through the documented PF_ColorParamSuite1 on a32bpc16×16owned composition. Existing and newly added instances are compared under None, sRGB, sRGB+Linearize, Rec709Gamma2.4, Rec709+Linearize, AdobeRGB, AdobeRGB+Linearize and DisplayP3.16renders succeed; every six-swatch fingerprint is exactly identical across all16stages, including worker-thread callbacks. **Rejected as an encoding resolver**: this cannot distinguish transfer or primaries. No default Auto/guessed sRGB behavior is shipped.

Exact evidence: `../automatic-color-probe-20261018-v3/{host-report.txt,process-error.log,verification.json,installation.json,cleanup.json}`. First two diagnostic attempts fail before color rendering because the effect is absent from the scripting effect list despite entry registration success. Fresh packaging with executable-matching resource basename plus support URL registers and renders; this joint change does not isolate a single root cause. All attempt bytes/logs preserved. After the owned process exits, only hash-inventoried diagnostic effect/helper bundles are archived; original installed effects are unchanged.

Next: an authoritative main-thread project-profile bridge must prove freshness for immediate scripted render, project-space changes and fresh aerender; idle snapshots alone remain insufficient evidence. Auto remains required/open rather than silently replaced by a numeric swatch guess.

## AE-AUTO-LIFECYCLE-001 — actual AE25.6 callback order

Baseline585b9da; diagnostic-only additions in `tools/ae_color_probe/` and `tools/ae_host_runner/`, both new modes defaultOFF. The product effect and installed Current bundle are unchanged. An owned32bpc16×16composition switches eight project working-space/Linearize settings, rendering an existing and newly added diagnostic effect in each setting (16renders) immediately from one script. The diagnostic has actual flat sequence data, GET_FLATTENED support, SEND_UPDATE_PARAMS_UI, MFR and dynamic-flag support. The owned AEGP helper also registers an RQM listener and traces idle callbacks.

Observed:0idle callbacks during the synchronous script; RQM registration returns0but no job/item notification arrives on this path. SEQUENCE_SETUP and GET_FLATTENED occur9times on main (initial instance +8new instances), while all16SmartPreRender/SmartRender callbacks run off-main. QUERY_DYNAMIC_FLAGS also runs off-main; RESETUP/SETDOWN are seen on workers. Thus initial snapshot transport is observable, but neither idle nor these serialization observations supplies an established fresh project descriptor for every changed-setting render. No main-thread API is called from a worker to bypass this limitation.

Evidence: external `../color-lifecycle-20261018/{host-report.txt,process-error.log,verified-lifecycle.json,installation.json}`. Reproduce summary with `tools/ae_color_probe/verify_lifecycle.py ROOT NEW_OUTPUT`. This is one host/path diagnostic, not proof that every SDK mechanism or later AE version is impossible.

Senior architecture advice was requested under the user's standing authorization, using GPT-6-astra, read-only. It found no supported universal freshness barrier in the reviewed SDK25.6 interfaces. RQM does not document preview coverage or a before-first-render/main-thread barrier; GET_FLATTENED documents state transfer, not sampling of changed project color. Primary references: [MFR selectors](https://ae-plugins.docsforadobe.dev/effect-details/multi-frame-rendering-in-ae/#implications-to-command-selectors-with-multi-frame-rendering), [AEGP threading](https://ae-plugins.docsforadobe.dev/aegps/implementation/#threading), [RQM suite](https://ae-plugins.docsforadobe.dev/aegps/aegp-suites/#render-queue-monitor-suite).

Alternative under investigation: documented built-in Color Profile Converter before/after FSTR, with an explicitly fixed internal color space. This is an effect-stack workflow, not a standalone native Auto implementation. Adobe exposes separate input/output Linearize controls; their automatic relationship to project Linearize is not established. Exact whole-stack Mix0/neutral copy, negative/HDR values, gamut mapping and alpha require separate checks. No preset/stack is shipped or claimed accepted. [Adobe utility effects](https://helpx.adobe.com/after-effects/desktop/apply-effects-and-animation-presets/list-of-effects/utility-effects.html#color-profile-converter-effect).

The user answered explicitly: sRGB by default with manual choice; Auto is not needed at all. SRGB-DEFAULT-001 above supersedes this investigation; no converter workflow or Auto bridge is to be promoted.

Converter inventory: two unstarted attempts are preserved outside Git under `../converter-inventory-20261018*`. After archiving all prior hash-verified inert test helpers, a fresh single-helper process successfully inventories the built-in effect `ADBE ProfileToProfile`. Its input/output profile selectors are numeric and not expression-enabled; both Linearize controls default0and allow expressions; Intent defaults3and Black Point Compensation1. This closes script-access inventory only: converter pixel transport, profile-index-to-name mapping, linearization behavior and whole-stack roundtrip remain NOT_RUN. Exact report: `../converter-inventory-20261018-v2/host-report.txt`; source script `tools/ae_color_probe/converter_inventory.jsx.in`. A nonce-derived bundle identifier is added for provenance/isolation; single-helper startup succeeds, but the joint isolation/identity changes do not establish one specific cause of skipped startup. All test helpers are archived after process exit. Existing user-installed effects and functional Smart Current bundle are unchanged.
