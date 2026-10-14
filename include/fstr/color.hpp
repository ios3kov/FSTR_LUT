#pragma once
#include "fstr/core.hpp"

namespace fstr {
// Explicit linear RGB in finite float input range, sRGB/Rec709 primaries;
// double intermediates retain precision. Caller owns transfer conversion/output bounds.
// This is a grading operator, not SDR -> camera Log or recovered scene radiance.
struct GradeSettings {
    double exposure_stops = 0.0; // [-10,10], uniform exp2 gain
    double contrast = 1.0;       // [0.1,3], channelwise signed-log contrast
    double saturation = 1.0;     // [0,2], linear Rec709 luminance interpolation
};
bool grade_is_neutral(const GradeSettings&) noexcept;
void validate_grade(const GradeSettings&);
RGB grade_linear(RGB, const GradeSettings&);
} // namespace fstr
