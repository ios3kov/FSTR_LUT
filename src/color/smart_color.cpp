#include "fstr/smart_color.hpp"
#include <cmath>
#include <limits>

namespace fstr {
namespace {
void unit(double x) {
    if(!std::isfinite(x)||x<0||x>1)throw std::invalid_argument("Smart mask/amount requires finite [0,1]");
}
void finite_rgb(RGB v) {
    for(double x:{v.r,v.g,v.b})
        if(!std::isfinite(x)||std::abs(x)>std::numeric_limits<float>::max())
            throw std::invalid_argument("Smart RGB exceeds finite float range");
}
}
void validate_smart_balance(const SmartBalance& b) {
    if(!std::isfinite(b.exposure_stops)||std::abs(b.exposure_stops)>.5)
        throw std::invalid_argument("Smart exposure requires finite [-0.5,0.5] stops");
    for(double x:{b.gains.r,b.gains.g,b.gains.b})
        if(!std::isfinite(x)||x<.9||x>1.1)throw std::invalid_argument("Smart balance gains require finite [0.9,1.1]");
}
RGB prepare_smart_color(RGB v,const SmartBalance& b) {
    validate_smart_balance(b);finite_rgb(v);
    if(b.exposure_stops==0&&b.gains.r==1&&b.gains.g==1&&b.gains.b==1)return v;
    const double g=std::exp2(b.exposure_stops);
    RGB out{v.r*g*b.gains.r,v.g*g*b.gains.g,v.b*g*b.gains.b};finite_rgb(out);return out;
}
RGB protect_skin_chroma(RGB b,RGB l,double weight,double amount) {
    unit(weight);unit(amount);finite_rgb(b);finite_rgb(l);
    const double p=weight*amount;if(p==0)return l;
    const RGB d{l.r-b.r,l.g-b.g,l.b-b.b};
    const double y=.2126*d.r+.7152*d.g+.0722*d.b;
    RGB out{l.r-p*(d.r-y),l.g-p*(d.g-y),l.b-p*(d.b-y)};
    finite_rgb(out);return out;
}
SmartLook::SmartLook(std::shared_ptr<const LUT> l,double s,double p):look_(std::move(l)),strength_(s),protection_(p) {
    unit(p);
    if(!std::isfinite(s)||s<0||s>3)throw std::invalid_argument("Smart look strength requires finite [0,3]");
}
RGB SmartLook::apply(RGB b,double mask) const {
    finite_rgb(b);unit(mask);
    if(!look_||strength_==0)return b;
    const auto encoded=look_->sample({linear_to_srgb(b.r),linear_to_srgb(b.g),linear_to_srgb(b.b)});
    const RGB l{srgb_to_linear(encoded.r),srgb_to_linear(encoded.g),srgb_to_linear(encoded.b)};
    const RGB styled{b.r+strength_*(l.r-b.r),b.g+strength_*(l.g-b.g),b.b+strength_*(l.b-b.b)};
    return protect_skin_chroma(b,styled,mask,protection_);
}
} // namespace fstr
