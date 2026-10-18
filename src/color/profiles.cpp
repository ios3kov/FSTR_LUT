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
namespace {
constexpr double arri4_a=(262144.-16.)/117.45,arri4_b=928./1023.,arri4_c=95./1023.;
const double arri4_s=7*std::log(2.)*std::exp2(7-14*arri4_c/arri4_b)/(arri4_a*arri4_b);
const double arri4_t=(std::exp2(14*(-arri4_c/arri4_b)+6)-64)/arri4_a;
using Matrix=std::array<std::array<double,3>,3>;
// Derived from published xy primaries; column RGB vectors, no gamut clipping.
constexpr Matrix awg3{{{1.6175234363068063,-.53728662218829404,-.080236814118512523},{-.07057274089781608,1.3346130623303278,-.26404032143251172},{-.021101728042793505,-.22695387521826652,1.2480556032610601}}};
constexpr Matrix awg4{{{1.8931234426527115,-.78088150361692099,-.11224193903579102},{-.20570035825236616,1.3402574893837529,-.13455713113138659},{-.012705742927271422,-.15218487579620837,1.16489061872348}}};
constexpr Matrix vgamut{{{1.8065758837249346,-.6956972740836973,-.11087860964123689},{-.17009034310240004,1.3059552160953636,-.13586487299296371},{-.025205784016640302,-.15446832936057378,1.1796741133772144}}};
// ACES AP1 D60 -> XYZ -> Bradford D65 -> linear Rec709.
constexpr Matrix ap1{{{1.705050992657984,-.62179212065700451,-.083258872000978548},{-.13025641750704364,1.140804736575401,-.01054831906835808},{-.024003356804618022,-.12896897606497057,1.1529723328695889}}};
RGB multiply(const Matrix&m,RGB x){return {finite(m[0][0]*x.r+m[0][1]*x.g+m[0][2]*x.b),finite(m[1][0]*x.r+m[1][1]*x.g+m[1][2]*x.b),finite(m[2][0]*x.r+m[2][1]*x.g+m[2][2]*x.b)};}
}
double source_channel_to_linear(double x,SourceProfile p){
 finite(x);double value=0;
 switch(p){
  case SourceProfile::SonySLog3Cine:return slog3_to_linear(x);
  case SourceProfile::ArriLogC3EI800:value=x>5.367655*.010591+.092809?(std::pow(10.,(x-.385537)/.247190)-.052272)/5.555556:(x-.092809)/5.367655;break;
  case SourceProfile::ArriLogC4:value=x>=0?(std::exp2(14*(x-arri4_c)/arri4_b+6)-64)/arri4_a:x*arri4_s+arri4_t;break;
  case SourceProfile::PanasonicVLog:value=x<.181?(x-.125)/5.6:std::pow(10.,(x-.598206)/.241514)-.00873;break;
  case SourceProfile::ACEScct:value=x<=.155251141552511?(x-.0729055341958355)/10.5402377416545:x>=(std::log2(65504.)+9.72)/17.52?65504.:std::exp2(x*17.52-9.72);break;
  case SourceProfile::ACEScg:case SourceProfile::LinearRec709:return x;
  default:throw UnsupportedColorProfile("Explicit source profile required");
 }return finite(value);
}
double linear_to_source_channel(double x,SourceProfile p){
 finite(x);double value=0;
 switch(p){
  case SourceProfile::SonySLog3Cine:return linear_to_slog3(x);
  case SourceProfile::ArriLogC3EI800:value=x>.010591?.247190*std::log10(5.555556*x+.052272)+.385537:5.367655*x+.092809;break;
  case SourceProfile::ArriLogC4:value=x>=arri4_t?(std::log2(arri4_a*x+64)-6)/14*arri4_b+arri4_c:(x-arri4_t)/arri4_s;break;
  case SourceProfile::PanasonicVLog:value=x<.01?5.6*x+.125:.241514*std::log10(x+.00873)+.598206;break;
  case SourceProfile::ACEScct:value=x<=.0078125?10.5402377416545*x+.0729055341958355:(std::log2(x)+9.72)/17.52;break;
  case SourceProfile::ACEScg:case SourceProfile::LinearRec709:return x;
  default:throw UnsupportedColorProfile("Explicit source profile required");
 }return finite(value);
}
RGB source_to_linear_rec709(RGB x,SourceProfile p){
 x={source_channel_to_linear(x.r,p),source_channel_to_linear(x.g,p),source_channel_to_linear(x.b,p)};
 switch(p){case SourceProfile::SonySLog3Cine:return sony_cine_to_rec709(x);case SourceProfile::ArriLogC3EI800:return multiply(awg3,x);case SourceProfile::ArriLogC4:return multiply(awg4,x);case SourceProfile::PanasonicVLog:return multiply(vgamut,x);case SourceProfile::ACEScct:case SourceProfile::ACEScg:return multiply(ap1,x);case SourceProfile::LinearRec709:return x;default:throw UnsupportedColorProfile("Explicit source profile required");}
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
