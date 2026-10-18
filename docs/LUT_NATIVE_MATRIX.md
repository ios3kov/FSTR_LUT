# External LUT native matrix — 2026-10-18

Scope: authored Cube, integer Flame3DL, IRIDAS.look and cineSpace.csp fixtures in AE25.6x101 on this Mac. This closes the external-import workflow, not every format dialect or artistic grading acceptance. No installer, push, release or model changes.

## Corrections

1. AE does not dispatch USER_CHANGED_PARAM when selecting the already active popup item. Keep named Custom at saved slot18; append a separate final Load LUT... action at19. A successful load returns selection18. Parameter types, disk IDs, existing look numbers and math remain unchanged.
2. Remove redundant FORCE_RERENDER: SDK AE_Effect.h748–762 says ARB + CHANGED_VALUE already rerenders and forced invalidation does not work well with Undo. Human UI verification in buildf2c3a3d598d8e15a restores prior filename and pixels after standard Undo.
3. A rejected file must restore the transient selection through a successful host callback plus return_msg notification. Returning error516 discarded that restoration in AE and left Look19 selected, preventing renders. Installed final build49235442fbda89fd changes only this failure path from the Undo-verified build.

## Checks and evidence boundaries

| Criterion | Result / exact artifact |
| --- | --- |
| Four format import/render | PASS authored fixtures in521d; repeated3DL/LOOK/CSP plus Cube Undo in f2c3a3d598d8e15a, host-v5/pixels.json <=1/255+1e-6 |
| Empty cancel | PASS521d host/cancel-empty, restores Natural <=1code |
| Existing cancel | PASS f2c3a3d598d8e15a host-v5/cancel-existing, retains CSP <=1code |
| Repeated Load action | PASS18209946a61ac2d5 and f2c3a3d598d8e15a, multiple native standard-dialog imports |
| Missing + moved-file relink + save/reopen | PASS18209946a61ac2d5 host-v3; missing export3018 then relocated exports3019, all valid pixels <=1code |
| Human GUI Undo | PASS f2c3a3d598d8e15a host-v5/undo-ui; real Edit Undo, filename Relocated.cube restored, output agrees with independent Inverse reference <=1code |
| Final malformed rejection / next render | PASS49235442fbda89fd host-v6: error notification with callback0; Look18, prior pixels after rejection and save/reopen, following valid Cube and relocated reopen <=1code |
| Focused native callbacks Release + ASan/UBSan | PASS49235442fbda89fd rollback-release-test.log / rollback-sanitize-test.log |
| Prior full suite |17Release +17ASan/UBSan on521d; not claimed rerun in full on the final candidate |
| General format dialects / Windows / artistic flat-SDR acceptance | NOT_RUN here; separate project work |

Artifacts remain outside the repository at ../lut-matrix-20261018 and ../lut-reload-20261018. Every own AE helper is nonce/project/effect guarded, and earlier helpers/installed versions are archived with inventories. Failed v1 double-dispatch harness, intermediate missing-Undo and failed invalid-file rollback are retained. UI-source callbacks were separately reviewed against SDK ColorGrid and Supervisor; handle COPY/COMPARE ownership remains unchanged. Trace uses existing opt-in FSTR_LUT_SCENE_TRACE and logs token/callback metadata, never source frames.
