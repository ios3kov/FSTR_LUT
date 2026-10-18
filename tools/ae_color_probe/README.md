# Owned color-parameter experiment

AE-AUTO-SWATCH-001: determine whether PF_ColorParamSuite1 exposes enough information to distinguish supported working-space encodings. This is a diagnostic32bpc identity effect, not an Auto implementation. It only emits constant parameter values; never input pixels. Do not apply it to user projects.

TargetSDK25.6 AE_EffectSuites.h312–333 explicitly states floating color parameters are in the effect's working space. Supervisor.cpp uses the suite after PF_CHECKOUT_PARAM during rendering. That does not specify the source colorimetry of default color parameters, so numerical probing alone is not yet a safe resolver.

Acceptance: compare both an existing instance across project working-space/Linearize changes and a newly created instance in each setting. Include unmanaged/sRGB/Rec709Gamma2.4 and wider-gamut negative controls. Identical fingerprints for differently encoded worlds, or dependence on instance creation history, rejects the method. Preserve fresh-process/aerender as separate checks. No guessed sRGB fallback and no product parameter/migration change based solely on this experiment.

Actual-host result,2026-10-18:16working-space/instance stages ×6swatches all alias exactly on AE25.6x101; callbacks report non-main-thread execution. The diagnostic is rejected as an Auto resolver. See `docs/COLOR_AUTO_FOLLOWUP.md` and external `../automatic-color-probe-20261018-v3/verification.json`. Executable-matching `.rsrc` plus support URL packaging registers and runs; old failed attempts remain preserved. The diagnostic effect and helper are archived after process exit, rather than left in the user plugin directory.
