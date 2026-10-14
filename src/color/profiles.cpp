#include "fstr/color.hpp"
#include <cmath>
#include <stdexcept>

namespace fstr {
namespace {
double finite(double value) {
    if(!std::isfinite(value))throw std::invalid_argument("Color transfer exceeds finite range");
    return value;
}
}
// Sony Technical Summary appendix pp6-7: normalized full-range code/1023.
double slog3_to_linear(double x) {
    finite(x);
    return finite(x>=171.2102946929/1023.0
        ? std::pow(10.0,(x*1023.0-420.0)/261.5)*.19-.01
        : (x*1023.0-95.0)*.01125/(171.2102946929-95.0));
}
double linear_to_slog3(double x) {
    finite(x);
    return finite(x>=.01125
        ? (420.0+261.5*std::log10((x+.01)/.19))/1023.0
        : (x*(171.2102946929-95.0)/.01125+95.0)/1023.0);
}
RGB sony_cine_to_rec709(RGB x) {
    // Derived from official xy primaries and D65 via RGB->XYZ, inverse Rec709->XYZ.
    // Columns multiply a column RGB vector. No clamp: wide-gamut colors can be negative.
    return {
        finite(1.6269474097290801*x.r-.5401385388696354*x.g-.08680887085944523*x.b),
        finite(-.17851552711487664*x.r+1.4179409274640793*x.g-.23942540034920257*x.b),
        finite(-.04443611500929752*x.r-.1959199661720156*x.g+1.240356081181313*x.b)
    };
}
double decode_host(double x,Transfer t) {
    finite(x);
    switch(t) {
        case Transfer::SRGB:return finite(srgb_to_linear(x));
        case Transfer::Linear:return x;
        case Transfer::Gamma24:return finite(std::copysign(std::pow(std::abs(x),2.4),x));
    }
    throw std::invalid_argument("Unknown host transfer");
}
double encode_host(double x,Transfer t) {
    finite(x);
    switch(t) {
        case Transfer::SRGB:return finite(linear_to_srgb(x));
        case Transfer::Linear:return x;
        case Transfer::Gamma24:return finite(std::copysign(std::pow(std::abs(x),1.0/2.4),x));
    }
    throw std::invalid_argument("Unknown host transfer");
}
} // namespace fstr
