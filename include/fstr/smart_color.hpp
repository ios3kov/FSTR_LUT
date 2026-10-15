#pragma once
#include "fstr/color.hpp"

namespace fstr {
// Research boundary: straight linear Rec709 RGB. Caller handles source decode,
// alpha, mask resampling, temporal context and output encoding. No AE state.
struct SmartBalance {
    double exposure_stops = 0;
    RGB gains{1,1,1}; // Positive RGB multipliers, not Kelvin or illuminant recovery.
};
void validate_smart_balance(const SmartBalance&);
RGB prepare_smart_color(RGB linear, const SmartBalance&);
// weight is a soft semantic mask after confidence/temporal processing, [0,1].
// Preserves Rec709 linear luminance of styled; does not promise gamut containment.
RGB protect_skin_chroma(RGB prepared, RGB styled, double weight, double amount);
class SmartLook final {
public:
    SmartLook(std::shared_ptr<const LUT> look, double strength, double protection);
    RGB apply(RGB prepared, double mask_weight) const;
private:
    std::shared_ptr<const LUT> look_;
    double strength_, protection_;
};
} // namespace fstr
