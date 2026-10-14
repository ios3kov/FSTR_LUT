# Color development prototype

First native target: this Mac, After Effects2025/25.6, arm64. This is a development build. Windows, real-camera footage validation and full product readiness are separate checks.

Use **Already converted / SDR** when AE has already converted footage into the project color space. Use **Sony S-Log3 / S-Gamut3.Cine** only when the buffer still contains unconverted, full-range Sony Log samples. AE input color conversion must not run first in that case. Selecting Sony on already converted footage applies the transform twice. Other camera Log curves/gamuts are not interchangeable with this profile.

**AE Pixel Encoding** describes the samples the effect receives and returns, including project linearization. Select sRGB, Linear Rec709/sRGB or Rec709Gamma2.4 to match the project. Gamma2.4 is not the Rec709camera OETF. Unsupported/unknown project encoding is not guessed. Neutral ordinary SDR passes through exactly before processing; choosing Sony with neutral controls still normalizes its curve/gamut.

Exposure is measured in stops. Temperature and Tint are manual creative color trims: positive Temperature warms, positive Tint shifts toward magenta; zero preserves the original balance. They are not Kelvin or automatic scene correction. Contrast is a log-style grading control around a fixed0.18 linear pivot; it can move black and change color. Saturation preserves numerical linear luminance. HighlightRolloff optionally compresses luminance above0.6 smoothly; it does not recover missing detail or bring every wide-gamut color into SDR. Use32bpc to retain negative/above-one results; integer output has its normal representable range.

WarmFilm is the original, frozen version1 project look. LookAmount controls only that look. Its tetrahedral LUT samples clamp to the LUT domain; use LookAmount0 when extended values must pass without this artistic mapping. Mix fades the complete effect, including Sony normalization, against the original appearance in linear host output coordinates; Mix0 is the exact original samples. No denoise/sharpen/model processing runs in this prototype.

No frame-order dependent analysis or automatic exposure/white-balance changes. LUT/sample math, technical profile normalization and artistic tuning are evaluated separately. A known-space test fixture is useful for checking math; actual flat source and camera footage are needed for final artistic acceptance.

Build with an external SDK; Adobe headers/source are not redistributed:

```sh
cmake -S . -B build-ae -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 -DFSTR_AE_SDK_ROOT=/path/to/SDK
cmake --build build-ae
ctest --test-dir build-ae --output-on-failure
```

The native callback test loads the actual built executable with dlopen into an owned fixture. It is not the AfterEffects loader. Development bundle, source BuildID, exact binary/resources digests and actual host checks are recorded separately in STATUS/Evidence. Installation may only add a uniquely owned test bundle; preserve all existing plugins/projects.
