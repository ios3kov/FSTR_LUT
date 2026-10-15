# Owned AE acceptance runner — test tool only

This separate macOS AEGP bundle is not linked into or shipped with the color effect. It executes one compiled, reviewed JSX job on an idle callback only when FSTR_LUT_OWNED_TOKEN exactly matches its compiled nonce. Every JSX job must independently check that nonce and reject a pre-existing project before mutation. No retries after an unknown script effect.

Build with the external SDK and absolute owned script path:

```
cmake -S tools/ae_host_runner -B /new/owned/job/helper-build \
  -DFSTR_AE_SDK_ROOT=/external/ae25.6_61.64bit.AfterEffectsSDK \
  -DFSTR_RUNNER_SCRIPT=/new/owned/job/owned-test.jsx \
  -DFSTR_RUNNER_NONCE=<fresh-unique-nonce> -DCMAKE_BUILD_TYPE=Release
cmake --build /new/owned/job/helper-build
```

Configure the JSX template with a fresh absolute output directory and the same nonce. The renderer and references must share the declared transfer, source profile, alpha policy and dimensions. Set tolerances before inspecting host outputs; store actual project/output settings. A completed render is not an image comparison. Existing templates may append Compositing Options to the effect; verify the named custom controls, not a fixed total property count.

Install only under a distinct test helper name. Before replacing/removing a prior helper, compare the entire file inventory with its ownership manifest and archive it into a new owned path. Preserve all unrelated plugins/projects/preferences/templates. Launch a separate `After Effects -m` process with the nonce environment variable; never forward to a user session. The helper is inactive without the matching nonce.

The idle callback marks the job started before calling AEGP_ExecuteScript. It acquires MemorySuite before requesting its optional UTF-8 error handle; size/lock reads are bounded to4096 bytes, successful locks are unlocked, every returned handle is freed, and suites are released. Cleanup errors are logged and cannot establish PASS. A zero ExecuteScript return alone is not script/renderer acceptance. Guarded JSX writes its own report, balances suppression and schedules empty-project quit after the SDK caller has returned. Callbacks execute only on the host's main/idle path; no render hook or process-wide injection is used.

Tests may change only the owned project and owned output module, never global color settings/output templates. A failing source transport calibration is a harness failure, not evidence of an effect defect. Read exact logs and saved state before another attempt. Script assertions, exported 8-bit PSDs, expression samples, and actual float render worlds have distinct scopes. Preserve negative attempts.

Archive the exact helper after host work ends. It is not a product dependency. The distributed effect contains no Python, inference runtime, SDK helper or frame diagnostics.

## Smart Color acceptance template

`smart_acceptance.jsx.in` accepts one JSON replacement `FSTR_SMART_HOST_CONFIG_JSON` with `nonce`, `folder`, and `match` (extra provenance fields allowed). Use a fresh owned folder containing156opaque540x960 PNGs named `frames/frame000.png` through `frame155.png`, interpreted as sRGB25fps. It tests a distinctly named trial effect so the installed original remains untouched. It renders On/Off48/50/54/55/56 at8/16/32bpc, saves/reopens state, and exits only its nonce-owned empty process. Preserve exact source, bundle, output and PSD precision; a Photoshop8-bit export from16/32bpc includes conversion/dither. A separate pre-export Difference comparison is needed before attributing1LSB export differences to the effect. No normal-speed perceptual acceptance is implied.
