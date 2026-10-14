#include "fstr/color.hpp"
#include <cmath>
#include <limits>

namespace fstr {
namespace {
RGB shoulder(RGB v,double amount) {
    const double y=.2126*v.r+.7152*v.g+.0722*v.b,k=.6;
    if(amount==0||y<=k)return v;
    const double compressed=k+(1-k)*(y-k)/(1-k+y-k);
    const double gain=(1-amount)+amount*(compressed/y);
    return {v.r*gain,v.g*gain,v.b*gain};
}
}
ColorProcessor::ColorProcessor(const ColorSettings& s,Transfer t,Primaries p,Alpha a)
    :settings_(s),transfer_(t),alpha_(a),bypass_(false) {
    validate_grade(s.grade);
    for(double amount:{s.mix,s.shoulder,s.look_amount})
        if(!std::isfinite(amount)||amount<0||amount>1)throw std::invalid_argument("Color strength must be finite [0,1]");
    bypass_=s.mix==0||(s.source==SourceProfile::Host&&grade_is_neutral(s.grade)&&s.shoulder==0&&(!s.look||s.look_amount==0));
    if(!bypass_&&(p!=Primaries::SRGB_Rec709||
       (t!=Transfer::SRGB&&t!=Transfer::Linear&&t!=Transfer::Gamma24)||
       (a!=Alpha::Straight&&a!=Alpha::Premultiplied)||
       (s.source!=SourceProfile::Host&&s.source!=SourceProfile::SonySLog3Cine)))
        throw UnsupportedColorProfile("Explicit host Rec709 and supported source profile required");
}
std::array<float,4> ColorProcessor::apply(std::array<float,4> pixel) const {
    if(bypass_)return pixel;
    const auto& s=settings_;const double alpha=pixel[3];
    if(!std::isfinite(alpha)||alpha<0||alpha>1)throw std::invalid_argument("Invalid color alpha");
    if(alpha==0)return pixel;
    RGB raw{pixel[0],pixel[1],pixel[2]};
    if(alpha_==Alpha::Premultiplied){raw.r/=alpha;raw.g/=alpha;raw.b/=alpha;}
    RGB work;
    if(s.source==SourceProfile::SonySLog3Cine)work=sony_cine_to_rec709({slog3_to_linear(raw.r),slog3_to_linear(raw.g),slog3_to_linear(raw.b)});
    else work={decode_host(raw.r,transfer_),decode_host(raw.g,transfer_),decode_host(raw.b,transfer_)};
    work=shoulder(grade_linear(work,s.grade),s.shoulder);
    if(s.look&&s.look_amount>0) {
        auto mapped=s.look->sample({encode_host(work.r,Transfer::SRGB),encode_host(work.g,Transfer::SRGB),encode_host(work.b,Transfer::SRGB)});
        RGB looked{decode_host(mapped.r,Transfer::SRGB),decode_host(mapped.g,Transfer::SRGB),decode_host(mapped.b,Transfer::SRGB)};
        work={work.r+s.look_amount*(looked.r-work.r),work.g+s.look_amount*(looked.g-work.g),work.b+s.look_amount*(looked.b-work.b)};
    }
    if(s.mix<1) {
        RGB base{decode_host(raw.r,transfer_),decode_host(raw.g,transfer_),decode_host(raw.b,transfer_)};
        work={base.r+s.mix*(work.r-base.r),base.g+s.mix*(work.g-base.g),base.b+s.mix*(work.b-base.b)};
    }
    const double mult=alpha_==Alpha::Premultiplied?alpha:1.0;
    double values[]{encode_host(work.r,transfer_)*mult,encode_host(work.g,transfer_)*mult,encode_host(work.b,transfer_)*mult};
    for(std::size_t j=0;j<3;++j) {
        if(!std::isfinite(values[j])||std::abs(values[j])>std::numeric_limits<float>::max())throw std::invalid_argument("Color output exceeds finite float range");
        pixel[j]=static_cast<float>(values[j]);
    }
    return pixel;
}
} // namespace fstr
