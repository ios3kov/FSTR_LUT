# Camera metadata — authorized addition to the color MVP

Direct user request2026-10-14: automatically read useful embedded camera metadata when present. Existing color processing, parameter IDs and deferred AI/performance work remain unchanged. Initial supported source is Sony NonRealTimeMeta XML in MP4/MOV ISO-BMFF meta/xml boxes, including moov/udta/meta containers. rtmd, sidecars, arbitrary UUID XML, nested compositions and image sequences are not this slice.

| ID | Observable acceptance |
| --- | --- |
| META-01 | Independent C++ reader recognizes manufacturer/model, CaptureGammaEquation, CaptureColorPrimaries, CodingEquations from supported source boxes; no frame decoding |
| META-02 | Skip mdat/free, maximum4MiB reads/256KiB XML/4096boxes/bounded container depth; invalid/truncated/conflicting XML/container/namespace fails explicitly; no external entities |
| META-03 | No GPS, serial number, original footage or full private XML in metadata result/logs/repository |
| META-04 | Effect Controls custom DRAW resolves the current direct footage source through exact SDK suites, automatically displays recognized declarations; no file operations in render; cached results refresh on source/path/mtime/size change or control click |
| META-05 | Missing file, proxy, unsupported source/profile and absent metadata show a truthful manual fallback; no guessed profile or automatic changes to render parameters |
| META-06 | Existing persistent IDs1..11 and saved grading survive; new UI-only NO_DATA control uses12; metadata never enters render sequence/settings/cache and does not change image result |
| META-07 | Inspect actual owned AE display with supplied Sony source; test cached redraw, source replacement, fallback and existing color regression; build/sanitizer and core tests stay separate from actual host acceptance |

Recognition reports declarations in the source file, not camera-setting authentication or the incoming effect pixel encoding. SDK25.6 PFInterfaceSuite1 documents GetEffectLayer for EVENT::DRAW (among other selectors); UPDATE_PARAMS_UI is not used for discovery. The public SDK does not establish all assigned-input/PreserveRGB/linearization/prior-effect conversions. Source Profile and AE Pixel Encoding therefore remain explicit, preventing an automatic double Log conversion. Recognition is automatic when the metadata control is drawn; there is no promise of scanning while ECW is closed.

Filesystem reading is bounded by bytes, not a wall-clock deadline on slow/disconnected volumes. Only local regular media files are the tested target. Results cache at most128 small records; frame data is never cached by this feature. tinyxml2 v11.0.0 is pinned to9148bdf719e997d1f474be6bcc7943881046dba1 with its zlib license and hashes. Windows, real S-Log metadata/footage and non-Sony metadata remain separately unverified.
