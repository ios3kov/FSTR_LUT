# Complex long gaps: ordinary LUT — accepted steering2026-10-15

User explicitly clarified: ordinary non-Smart grading applies to difficult long gaps only. Preserve validated short interpolation. Do not extend tracking, hold old regions or invent head positions to bridge long unsupported intervals.

Existing bounded recovery already implements this behavior: raw current detections remain usable; a missing region may be restored only with both confirmed endpoints and every current-image appearance check, for at most two missing frames. Otherwise no mask is produced and ColorProcessor receives protection0. For multiple people, this is per region: an unsupported person does not disable confidently detected other people.

No runtime algorithm change was necessary for this clarification. Added actual module regression for real54/55/56, comparing Smart On with the explicit neighboring window against Smart Off. Every pixel matches on all three frames; alpha retained and checkout resources balanced. Actual short-gap50 still changes504pixels through verified recovery. Eight CTest targets PASS; existing bundle bytes unchanged. This is actual Vision/module execution through owned callbacks, not an AE host run.

The earlier plan to extend tracking through long dropouts is cancelled by this steering. Long-gap ordinary-LUT output is now intended behavior, not a missing recovery feature. The earlier unconditional promise of a smooth transition is not a verified claim: no stale-mask fade was added. Temporal appearance at the boundary still needs normal-speed visual/AE validation; exact unsupported-frame fallback takes precedence over retaining invalid masks. Short interpolation is unchanged.

NOT RUN: actual AE UI, save/reopen, full host MFR/ROI and continuous perceptual transitions. Installed plugin unchanged. No install, push, merge or release.
