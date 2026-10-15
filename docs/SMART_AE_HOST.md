# Actual AE Smart trial — 2026-10-15

Source checkpoint db23cfb; host AE25.6x101, native arm64 Mac. Built isolated source archive, changing only display/match/bundle names in plugin.cpp, PiPL and plist. Processing code unchanged; trial source buildID e6e935e2c4727467. Trial match `FSTR LUT Smart Host db23cfb`, visible name `FSTR-LUT Smart Host Test`. Distinct entry avoids duplicate matching with the original installed effect. This is trial identity acceptance, not original-match project migration.

## Observed results

- PASS actual loader/parameter creation. Checkbox defaultOff, On/Off states and Look preserved across save and reopen of owned project.
- PASS renders with project8/16/32bpc on known sRGB PNG sequence derived from supplied footage,540x960,25fps. Menu3 is Matrix - Green Code, intentionally strong enough to reveal protection. Frame48 automatic region and50short-gap recovery visibly affect only face area in8-bit output (560/495changed pixels). Alpha equal.
- PASS intended long-gap ordinary LUT at54/55/56:8bpc On/Off exports exact. Separate16/32project Difference composites, calculated before output conversion, export all-zero RGB. Individual16/32project Photoshop8-bit exports differ by at most1LSB across the image; do not call those files byte-identical or claim exact floating-point equality from8-bit Difference output.
- PASS cropped ROI:270x960 output equals columns135:405 of full540x960 On48 output exactly.
- PASS complete156frame AE side-by-side render and MP4 encode/full decode. Four contact sheets plus enlarged48/50comparison inspected. Static sheets cannot certify absence of continuous flicker or halos under all movement.
- PASS independent aerender invocation with MFR ON/CPU50%, saved project, frames48–56: nine images exactly equal main8bpc output. This is MFR-enabled smoke evidence; cache participation/actual overlapping callbacks were not instrumented, so not a concurrency stress proof.
- PASS real original C2098.MP4 import and two On/Off renders after resolving its moved path. AE reports2160x3840,50fps; no extra rotation needed. Source profile remains assumed Gamma2.4 SDR, not newly certified camera encoding. Original-file render is separate from the prepared sequence numeric oracle.

## Preserved and delivered

Initial user AE process82247 (birth2026-10-15 19:09:30 local) remained running; no commands sent to it. All three owned -m processes exited. Original installed five-file bundle hash unchanged. Temporary nonce-gated helper verified and archived; distinct trial effect remains installed. It appears in a new AE launch under FSTR.Tools. Do not close the user's current project to make it visible.

Trial package: outputs/FSTR-LUT-Smart-AE-Trial-2026-10-15/FSTR-LUT-Smart-Trial.zip. Saved project: work/smart-ae-host-2026-10-15/final/smart-motion.aep (local media dependencies, not portable). Comparison: work/smart-ae-host-2026-10-15/AE-Smart-On-Off.mp4; leftOff/rightOn, Matrix preset. No main/push/merge/release or original installed replacement.

## Limitations and truthful gate

NOT RUN: continuous normal-speed perceptual approval, old user-project migration to original match identity, Undo/UI gestures, broad alpha/HDR/PAR source corpus and Windows. Spatial softness inspected in selected face frames; no blanket no-flicker claim. Long unsupported gaps intentionally fall back to ordinary LUT, per user steering. Quality of transitions remains separate from functional render acceptance.

The second job completed all156motionframes, then failed importing the obsolete `/Users/os3kov/Downloads/C2098.MP4`. Its report is preserved. Found the named source inside Downloads/work/JT/Work/2026/10_oct/04_PartnINFO/Test/Sources; a fresh isolated job completed original-file and ROI checks and saved its project. No replay after unknown effects.

A limited local trial is available. Full release readiness is not established. Exact source/installed/output evidence: SMART_AE_HOST_EVIDENCE.json.
