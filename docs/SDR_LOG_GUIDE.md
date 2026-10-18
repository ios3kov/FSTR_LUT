# Color development prototype

First native target: this Mac, After Effects2025/25.6, arm64. This is a development build. Windows, real-camera footage validation and full product readiness are separate checks.

Use **Already converted / SDR** when AE has already converted footage into the project color space. Select a named **Input Profile** only if the pixels reaching this effect still use that exact source encoding. The available explicit pairs are Sony S-Log3/S-Gamut3.Cine, ARRI LogC3 **EI800**/AWG3, ARRI LogC4/AWG4, Panasonic V-Log/V-Gamut, ACEScct/AP1 and linear ACEScg/AP1. AE input color conversion must not run first when using an override. Choosing a camera/ACES override on already converted pixels applies normalization twice. Other curves/gamuts or ARRI exposure-index definitions are not interchangeable. ACES entries normalize source data into linear Rec709; this is not an ACES output/display transform.

**AE Pixel Encoding** describes the output/project samples. New effects start at **sRGB**. Manual **Linear Rec709/sRGB** and **Rec709 Gamma2.4** choices remain in collapsed **Advanced**; use them when the actual host workflow requires it. Auto is not provided. Gamma2.4 is not the Rec709camera OETF. Neutral ordinary SDR passes through exactly; named source overrides normalize curve/gamut even with neutral grading controls. Metadata is read-only and never selects an override: XMP camera names alone do not establish a Log profile. Full scope/precision and remaining formats: [INPUT_PROFILES.md](INPUT_PROFILES.md).

Exposure is measured in stops. Temperature and Tint are manual creative color trims: positive Temperature warms, positive Tint shifts toward magenta; zero preserves the original balance. They are not Kelvin or automatic scene correction. Contrast is a log-style grading control around a fixed0.18 linear pivot; it can move black and change color. Saturation preserves numerical linear luminance. HighlightRolloff optionally compresses luminance above0.6 smoothly; it does not recover missing detail or bring every wide-gamut color into SDR. Use32bpc to retain negative/above-one results; integer output has its normal representable range.

**Natural** is the neutral look; select a cinematic look for creative grading. **Look Amount** starts at100%, its slider displays0–100and manual entry allows up to300%. The retired WarmFilm v1 stays frozen only for old projects. LUT interpolation is tetrahedral and the lookup clamps to its domain; choose Natural/LookAmount0 when extended values should avoid that artistic mapping. **Mix** fades the complete effect, including source normalization, against the original appearance in linear host output coordinates; Mix0 is exact original samples. No denoise or sharpening is added. Experimental native Smart and its separate analysis cache are described in AUTOMATIC_SMART.md; it does not add Python to the effect.

No frame-order dependent analysis or automatic exposure/white-balance changes. LUT/sample math, technical profile normalization and artistic tuning are evaluated separately. A known-space test fixture is useful for checking math; actual flat source and camera footage are needed for final artistic acceptance.

Build with an external SDK; Adobe headers/source are not redistributed:

```sh
cmake -S . -B build-ae -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DFSTR_AE_SDK_ROOT=/path/to/SDK
cmake --build build-ae
ctest --test-dir build-ae --output-on-failure
```

The native callback test loads the actual built executable with dlopen into an owned fixture. It is not the AfterEffects loader. Development bundle, source BuildID, exact binary/resources digests and actual host checks are recorded separately in STATUS/Evidence. Installation may only add a uniquely owned test bundle; preserve all existing plugins/projects.
