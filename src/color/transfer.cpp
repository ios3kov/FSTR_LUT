#include "fstr/core.hpp"
#include <array>
#include <cmath>
namespace fstr {
double srgb_to_linear(double x) {return x<=0.04045?x/12.92:std::pow((x+0.055)/1.055,2.4);}
double linear_to_srgb(double x) {return x<=0.0031308?x*12.92:1.055*std::pow(x,1.0/2.4)-0.055;}
namespace {
constexpr std::size_t samples=65536;
struct Tables {
    std::array<double,samples+1> decode{},encode{};
    Tables(){for(std::size_t i=0;i<=samples;++i){double x=double(i)/double(samples);decode[i]=srgb_to_linear(x);encode[i]=linear_to_srgb(x);}}
};
const Tables& tables(){static const Tables table;return table;}
double interpolate(const std::array<double,samples+1>& table,double x) {
    double position=x*double(samples);auto index=static_cast<std::size_t>(position);
    if(index==samples)return table[samples];
    double fraction=position-double(index);return table[index]+fraction*(table[index+1]-table[index]);
}
}
double srgb_to_linear_fast(double x) {
    if(x<=.04045 || x>1 || !std::isfinite(x))return srgb_to_linear(x);
    return interpolate(tables().decode,x);
}
double linear_to_srgb_fast(double x) {
    if(x<=.0031308 || x>1 || !std::isfinite(x))return linear_to_srgb(x);
    return interpolate(tables().encode,x);
}
}
