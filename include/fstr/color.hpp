#pragma once
#include "fstr/core.hpp"
#include <stdexcept>

namespace fstr {
// Explicit linear RGB in finite float input range, sRGB/Rec709 primaries;
// double intermediates retain precision. Caller owns transfer conversion/output bounds.
// This is a grading operator, not SDR -> camera Log or recovered scene radiance.
struct GradeSettings {
    double exposure_stops = 0.0; // [-10,10], uniform exp2 gain
    double contrast = 1.0;       // [0.1,3], channelwise signed-log contrast
    double saturation = 1.0;     // [0,2], linear Rec709 luminance interpolation
    double temperature = 0.0, tint = 0.0; // [-1,1], creative RGB gains; not Kelvin/CCT.
};
bool grade_is_neutral(const GradeSettings&) noexcept;
void validate_grade(const GradeSettings&);
RGB grade_linear(RGB, const GradeSettings&);

// Values0/1 are persistent. Append profiles; never reorder saved AE choices.
enum class SourceProfile { Host, SonySLog3Cine, ArriLogC3EI800, ArriLogC4, PanasonicVLog, ACEScct, ACEScg };
inline constexpr int source_profile_count=7;
inline constexpr const char* source_profile_keys[]{"host","sony-slog3-cine","arri-logc3-ei800","arri-logc4","panasonic-vlog","acescct","acescg"};
inline constexpr char source_profile_menu[]="Already converted / SDR|Sony S-Log3 / S-Gamut3.Cine|ARRI LogC3 EI800 / AWG3|ARRI LogC4 / AWG4|Panasonic V-Log / V-Gamut|ACEScct / AP1|ACEScg / Linear AP1";
double source_channel_to_linear(double code,SourceProfile);
double linear_to_source_channel(double linear,SourceProfile);
RGB source_to_linear_rec709(RGB code,SourceProfile);
double slog3_to_linear(double code);
double linear_to_slog3(double linear);
RGB sony_cine_to_rec709(RGB linear);
// Host/output transfer only; camera encoding is a separate source override.
double decode_host(double code, Transfer);
double encode_host(double linear, Transfer);
struct ColorSettings {
    SourceProfile source = SourceProfile::Host;
    GradeSettings grade;
    double shoulder = 0.0, look_amount = 1.0, mix = 1.0; // Look[0,3], others[0,1].
    std::shared_ptr<const LUT> look; // Immutable, encoded sRGB/Rec709 contract.
};
struct ColorContext {
    const std::atomic_bool* cancelled = nullptr;
    std::size_t max_pixels = 8388608; // Includes full3840x2160, no model memory.
};
class UnsupportedColorProfile final:public std::invalid_argument {
public:using std::invalid_argument::invalid_argument;
};
class ColorProcessor final {
public:
    ColorProcessor(const ColorSettings&,Transfer,Primaries,Alpha);
    bool bypass() const noexcept {return bypass_;}
    std::array<float,4> apply(std::array<float,4> rgba,double color_protection=0) const;
private:
    ColorSettings settings_;
    Transfer transfer_;
    Alpha alpha_;
    bool bypass_;
};
RenderResult render_color(const FrameWindow&, const ColorSettings&, const ColorContext&) noexcept;
} // namespace fstr
