# Cinema & Palettes — scope, 2026-10-15

Direct user requests: recognizable Hollywood-film looks, clear names; remove old Warm from new choices; Natural must be maximally natural; Teal & Orange plus the eight supplied color pairs. Strength has soft slider 0–100%, default100%, and typed amplification above100% (bounded to300%). This is an authorized addition to the existing color MVP, not restoration/AI/acceleration or a release.

## Observable acceptance

- CIN-01: Natural is old neutral value1, no LUT/tint/contrast added. With neutral manual settings, SDR copies exactly. Sony input still gets its explicit technical normalization. Look Amount0 bypasses the LUT only; manual grading/source normalization remain active.
- CIN-02: six film-inspired styles and nine named color pairs selectable, with visibly distinct results on C2098 and authored ramps/swatches at100%. Independent numerical comparisons support difference, not artistic similarity.
- CIN-03: old Warm removed from selectable menu. Persistent value2 is a disabled separator, preserving the frozen legacy renderer for old saved projects/animated values; it is never relabelled Natural. IDs1..12 and parameter order remain stable; new entries append. New demos contain no legacy Warm.
- CIN-04: Look Amount displays0–100%, defaults100%, accepts typed0–300%. Core accepts only finite[0,3]. Above100% extrapolates in existing host-linear mixing space and may produce extended values, preserved in32bpc and quantized/clamped in integer outputs. Mix and highlight rolloff remain[0,1]. No new silent clamp.
- CIN-05: all look assets versioned/frozen by catalog SHA256; load once in GlobalSetup, immutable shared snapshot per render, no render filesystem/AE UI. Same input/settings independent of order/thread, alpha unchanged.
- CIN-06: build, affected core/native/sanitizer tests, actual AE menu/strength/save-reopen/render and separate visual review bound to exact candidate. Old Warm compatibility and Natural exactness retained; actual host gaps remain explicit.

## Art direction and sources

The list is an editorial selection of recognizable references, not a measured popularity ranking. Film colors also depend on lighting, sets, exposure and scene; one global LUT cannot reproduce those conditions. These are original analytical recipes, with no downloaded studio/commercial LUT, extracted movie pixels, affiliation or exact-match claim. User supplied the eight additional pair names.

| Menu | Intended direction / source scope |
|---|---|
| Matrix - Green Code | Green virtual-world interpretation, restrained saturation. [Bill Pope interview, ASC](https://theasc.com/articles/flashback-the-matrix). |
| Mad Max - Desert Fire | Rich graphic daytime desert, gold/orange versus cyan-blue. [Colorist Eric Whipp's account](https://lowepost.com/casestudies/mad-max-fury-road-r9/). |
| Blade Runner 2049 - Neon | Original cool-neon/amber city interpretation; the film has multiple distinct palettes. [Roger Deakins interview, ASC](https://theasc.com/article/uncanny-valley-blade-runner-2049/). |
| Joker - Gotham | Original cyan-shadow/sodium-light urban interpretation. [Lawrence Sher interview](https://www.btlnews.com/awards/oscar-contender-joker-cinematographer-lawrence-sher/). |
| Dune - Arrakis | Restrained desert saturation, reduced blue, warm highlights. [Greig Fraser/Dave Cole, ASC](https://theasc.com/article/dune-fear-is-the-mind-killer/). |
| The Dark Knight - Steel | Author's cool urban/steel-blue interpretation; exact grading math UNKNOWN. [Wally Pfister interview, ASC](https://theasc.com/video/wally-pfister-asc-the-dark-knight-and-imax/). |
| Blockbuster Teal & Orange | Teal shadows / warm highlights. |
| Cyberpunk Neon Pink & Cyan | Cyan shadows / pink mids and lights. |
| Magic Purple & Gold | Violet shadows / golden highlights. |
| Vintage Red & Mint | Mint shadows / red highlights, mild lifted black. |
| Sunlight Blue & Yellow | Blue shadows / yellow lights. |
| Earthy Olive & Dusty Pink | Soft muted olive / dusty rose. |
| Luxury Navy & Gold | Deep blue shadows / gold lights. |
| Elegance Emerald & Rose Gold | Emerald shadows / warm pink highlights. |
| Calm Lavender & Sage | Pastel sage / lavender, lifted blacks. |

Sources accessed2026-10-15 support broad palette intentions only, not the numerical recipes or a transferable production LUT license. Recipes use luminance sigmoid, saturation and smooth tonal-zone tinting with bounded chroma compression in encoded sRGB, baked at33³. These global operations do not identify skin/objects or simulate film stock/grain. No automatic camera-profile change. Natural is deliberately not an automatic enhancement.

AE separator syntax `(-` and its persistent menu index are documented in the [AE SDK guide](https://ae-plugins.docsforadobe.dev/effect-basics/parameters/) and [Adobe dropdown documentation](https://helpx.adobe.com/after-effects/desktop/work-with-expressions/expression-language-reference/expression-language-reference.html); exact SDK25.6 PF_PopupDef is authority for types. Validate the actual menu separately in AE.

## Status

Implementation/verification in progress. Existing STATUS.md color, metadata, Log/HDR/Windows/video obligations are retained. No whole-product or film-match acceptance claimed.
