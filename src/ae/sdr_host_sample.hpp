#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace fstr::ae {
// AE can deliver bounded SDR white one nominal 16-bpc code above one.
// This tolerance belongs only to AutoPrepare's encoded host boundary, never
// to the general colour pipeline or the core's strict linear SDR contract.
inline double bounded_host_sdr(double encoded_straight) {
    constexpr double residue=1.0/32768.0;
    if(!std::isfinite(encoded_straight)||encoded_straight < -residue||encoded_straight > 1+residue)
        throw std::invalid_argument("Auto Prepare requires bounded SDR pixels");
    if(encoded_straight<0)return 0;
    if(encoded_straight>1)return 1;
    return encoded_straight;
}
// Recipe analysis excludes alpha<0.5; preserve that existing exclusion and
// leave malformed alpha for the core's explicit validation.
inline void normalize_sdr_anchor(float* rgba) {
    const double a=rgba[3];
    if(!std::isfinite(a)||a<.5||a>1)return;
    for(int c=0;c<3;++c){const double straight=double(rgba[c])/a;
        if(straight<0||straight>1||!std::isfinite(straight))rgba[c]=float(bounded_host_sdr(straight)*a);
    }
}
}
