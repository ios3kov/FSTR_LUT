#include "fstr/color.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace fstr {
bool grade_is_neutral(const GradeSettings& s) noexcept {
    return s.exposure_stops==0.0 && s.contrast==1.0 && s.saturation==1.0 && s.temperature==0.0 && s.tint==0.0;
}
void validate_grade(const GradeSettings& s) {
    if(!std::isfinite(s.exposure_stops)||s.exposure_stops < -10||s.exposure_stops > 10||
       !std::isfinite(s.contrast)||s.contrast < .1||s.contrast > 3||
       !std::isfinite(s.saturation)||s.saturation < 0||s.saturation > 2||
       !std::isfinite(s.temperature)||std::abs(s.temperature)>1||
       !std::isfinite(s.tint)||std::abs(s.tint)>1)
        throw std::invalid_argument("Invalid linear grading settings");
}
namespace {
// Fixed grading coordinates, not a claim of physical middle gray in SDR footage.
constexpr double pivot=.18, toe=.01;
double log_coordinate(double x) {return std::copysign(std::log1p(std::abs(x)/toe),x);}
double inverse_coordinate(double x) {return std::copysign(toe*std::expm1(std::abs(x)),x);}
double contrast_channel(double x, double amount) {
    const double p=log_coordinate(pivot);
    return inverse_coordinate(p+amount*(log_coordinate(x)-p));
}
}
PreparedGrade::PreparedGrade(const GradeSettings& s):settings_(s),neutral_(grade_is_neutral(s)) {
    validate_grade(s);
    if(s.exposure_stops!=0)exposure_=std::exp2(s.exposure_stops);
    if(s.temperature!=0||s.tint!=0) {
        const double red=std::exp2(.5*s.temperature),green=std::exp2(-.5*s.tint),blue=std::exp2(-.5*s.temperature);
        const double normal=.2126*red+.7152*green+.0722*blue;
        red_=red/normal;green_=green/normal;blue_=blue/normal;
    }
}
RGB grade_linear(RGB input,const GradeSettings& s){return PreparedGrade(s).apply(input);}
RGB PreparedGrade::apply(RGB input)const {
    const auto& s=settings_;
    if(!std::isfinite(input.r)||!std::isfinite(input.g)||!std::isfinite(input.b))
        throw std::invalid_argument("Nonfinite linear grading input");
    const double limit=std::numeric_limits<float>::max();
    if(std::abs(input.r)>limit||std::abs(input.g)>limit||std::abs(input.b)>limit)
        throw std::invalid_argument("Linear grading input exceeds declared float domain");
    if(neutral_) return input; // Also preserves signed zero without arithmetic.
    RGB result=input;
    if(s.exposure_stops!=0) {
        result.r*=exposure_;result.g*=exposure_;result.b*=exposure_;
    }
    if(s.temperature!=0||s.tint!=0) {
        result.r*=red_;result.g*=green_;result.b*=blue_;
    }
    if(s.contrast!=1) {
        if(s.natural_contrast) {
            const double y=.2126*result.r+.7152*result.g+.0722*result.b;
            // SDR tone scale: keep black/white fixed, preserve linear RGB ratios.
            // Nonpositive luminance and HDR highlights retain their values.
            if(y>0&&y<1) {
                const double p=std::log(pivot)-std::log1p(-pivot);
                const double z=p+s.contrast*(std::log(y)-std::log1p(-y)-p);
                const double mapped=z>=0?1/(1+std::exp(-z)):std::exp(z)/(1+std::exp(z));
                const double gain=mapped/y;
                result.r*=gain;result.g*=gain;result.b*=gain;
            }
        } else {
            result.r=contrast_channel(result.r,s.contrast);
            result.g=contrast_channel(result.g,s.contrast);
            result.b=contrast_channel(result.b,s.contrast);
        }
    }
    if(s.saturation!=1) {
        const double y=.2126*result.r+.7152*result.g+.0722*result.b;
        result.r=y+s.saturation*(result.r-y);
        result.g=y+s.saturation*(result.g-y);
        result.b=y+s.saturation*(result.b-y);
    }
    if(!std::isfinite(result.r)||!std::isfinite(result.g)||!std::isfinite(result.b))
        throw std::invalid_argument("Grading exceeds finite double range");
    return result;
}
} // namespace fstr
