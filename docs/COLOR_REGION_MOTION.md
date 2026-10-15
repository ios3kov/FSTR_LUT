# Appearance color regions and motion — 2026-10-15

## Explicit requirement change

The user clarified that same-colored hands/props may deserve the same color protection as faces. Skin-only semantic membership is no longer the acceptance criterion for this appearance mode. A strongly blurred head should be treated as a soft color object at any size, without facial holes. Motion stability must be tested. Earlier strict skin/occlusion failures remain historical evidence; they do not automatically constitute failures under the changed goal. Conversely, changed acceptance does not prove good appearance.

This is regional protection around automatically located heads/objects, not global selection of every warm-colored object in the image. No manual face selection. Research masks remain independent of AE. A separate native candidate is documented in SMART_COLOR_NATIVE.md; detector and recovery parity are not claimed.

## Implementation

`color_region_window` uses explicit3–5frame windows, scene-cut barriers and unique reciprocal association. It follows the existing24/64/160source-pixel size policy. Current geometry dominates; agreeing two-sided boxes contribute25% of position/size. A broad, smooth region exists independently of mesh/skin confirmation at any size. A strong isolated face-detector proposal may be retained without mesh through `allow_approximate=True`; body/head evidence remains a fallback. Multiple people remain separate observations.

Maximum regional weight0.65. Details modulate at most half of it and depend on source size and normalized local high-frequency energy. This energy is an uncalibrated detail-availability heuristic, not a universal physical blur classifier. Missing confirmation or no current detail gives a soft region with no facial holes. Neighboring sharp images cannot raise detail beyond current availability. They may support positioning, not invent visible detail.

Known hand/skin-only vetoes are deliberately not applied in this appearance mode. Similar-colored foreground objects within the same region can receive protection as requested. No claim of arbitrary-object recognition. Background spill, visual halos and harmful color shifts are still defects to assess. The prior strict pipeline is preserved for comparison and unchanged by default.

At most2missing observations can be bridged from unique two-sided raw observations, without crossing cuts or competing observations. Appearance mode removes the small-size restriction and deliberately omits semantic exclusions. It still requires compatible16×16RGB patches (mean error<=0.08). No recovered seed, global previousFrame or persistent identity history.

The existing native color kernel removes part of the LUT's chroma delta while retaining its linear Rec709 luminance delta. Mask0/absent LUT are exact bypasses. Internal function names still mention skin; this reuse does not turn the new regional mask into semantic skin recognition.

## Acceptance and verification scope

AR01: Same-region same-color hand/prop membership is allowed. Test that hand metadata does not veto the appearance mask; no detections must leave the image unselected.
AR02: Large blurred/low-detail objects receive a contiguous soft region; sharp neighbors cannot reintroduce detail. Detail masks cannot puncture more than half of the base region.
AR03: Explicit windows/cuts/order invariance and finite bounded outputs. Retain prior strict-mode regressions.
AR04: Analyze all156frames of the supplied6.24s excerpt at25fps,720×1280; render real C++ Teal & Orange output at360×640 with identity balance. Input Gamma2.4 SDR remains an explicit assumption, not metadata verification.
AR05: Compare the previous strict mask and the new appearance mask on identical pixels, LUT/strength and protection0.8. Measure flow-aligned mask change and changes in added color protection relative to LUT-only output. Report effect amplitude too, since different region weights/coverage can affect stability metrics.
AR06: Inspect rendered representative frames and full-sequence contact sheets; produce normal-speed full-frame and enlarged-head videos. Export/decoding/metrics do not establish continuous perceptual playback approval.

## Review and limitations

Independent review found that sharp neighbors could restore details on a blurred current confirmed anchor. Fixed by capping smoothed confidence at current availability; regression includes this case. Invalid nose/shoulder association geometry now fails validation instead of silently losing association. Eleven new tests plus40strict-mode tests pass; final runtime evidence is recorded separately.

NOT RUN: actual AE behavior/performance, real crowd crossings, all camera/input profiles and continuous perceptual playback approval. Detail-energy thresholds and region strength are provisional. Temporal metrics depend on imperfect optical flow; they cannot alone establish the absence of visible flicker. The installed plugin remains unchanged. No weights/footage upload or redistribution, push, merge or release.

## Full motion result

All156frames rendered; all reverse-order masks exact.145usable optical-flow aligned pairs: mean mask MAE old0.104144/new0.086940; mean added-color delta MAE old0.004930/new0.005081; mean effect amplitude old0.013602/new0.013087. Worst added-color MAE old0.011795/new0.018789. Thus smoother average mask does NOT establish improved color stability. Flicker acceptance remains open.

Inspected four fixed-region contact sheets covering156frames and enlarged54/55comparisons. Sheets show overall scene/color transitions but small face detail and late head movement outside the fixed crop limit assessment. Normal-speed overview and face-crop MP4s exist in the private motion-v2 directory; no continuous perceptual verdict. Source Gamma2.4 assumption retained. Private report: ../appearance-regions-2026-10-15/motion-v2/report.json. Neither this research nor the separate native scan establishes final AE behavior.
