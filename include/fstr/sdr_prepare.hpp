#pragma once
#include "fstr/color.hpp"
namespace fstr {
// Research prototype: caller supplies explicit anchors from ONE known SDR shot.
// No source-profile inference, previous-frame state, model, or AE dependency.
struct SdrPreparation {
    double contrast=1, brightness_bias=0, saturation=1, amount=1;
};
enum class SdrDecision { Prepared, Balanced, Ambiguous, Inconsistent };
struct SdrShotPreparation {
    SdrPreparation settings;
    SdrDecision decision=SdrDecision::Ambiguous;
    double low=0, median=0, high=0, flatness=0;
    std::size_t samples=0;
};
// At most16 borrowed anchors, same dimensions, bounded96x54 grid per frame.
// Sampled visible straight RGB must be in SDR[0,1]. Alpha<0.5 is excluded.
SdrShotPreparation analyze_sdr_shot(const std::vector<FrameView>&,const ColorContext& = {});
void validate_sdr_preparation(const SdrPreparation&);
// Explicit linear Rec709 input/output in[0,1]. Preserves luminance detail;
// chroma contracts towards gray when needed to fit the SDR cube, without RGB clipping.
RGB prepare_sdr_linear(RGB,const SdrPreparation&);
RenderResult render_sdr_preparation(FrameView,const SdrPreparation&,const ColorContext& = {}) noexcept;
}
