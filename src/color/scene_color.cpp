#include "fstr/scene_color.hpp"
#include <algorithm>
#include <cmath>
namespace fstr {
namespace {
double smooth(double a,double b,double x){const double t=std::clamp((x-a)/(b-a),0.,1.);return t*t*(3-2*t);}
ColorSettings settings(std::shared_ptr<const LUT> l,double s){ColorSettings v;v.look=std::move(l);v.look_amount=s;return v;}
}
void validate_scene_weights(const SceneWeights&w){double sum=0;for(float v:w){if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Invalid scene probability");sum+=v;}if(sum>1.000001)throw std::invalid_argument("Scene probability mass exceeds one");}
SceneLook::SceneLook(std::shared_ptr<const LUT> l,double s):base_(settings(std::move(l),s),Transfer::Linear,Primaries::SRGB_Rec709,Alpha::Straight),strength_(s){}
std::array<float,4> SceneLook::apply(std::array<float,4> p,const SceneWeights&w,bool enabled) const {
    // Off does not depend on inference availability or mask validity.
    const auto ordinary=base_.apply(p);
    if(!enabled||base_.bypass()||p[3]==0)return ordinary;
    validate_scene_weights(w);
    double mass=0;for(auto v:w)mass+=v;if(mass==0)return ordinary;
    // SDR residual only. Extended values keep the ordinary LUT result rather
    // than being clipped to fit the semantic branch.
    for(int c=0;c<3;++c)if(p[c]<0||p[c]>1||ordinary[c]<0||ordinary[c]>1)return ordinary;
    const double y=.2126*p[0]+.7152*p[1]+.0722*p[2];
    const double peak=std::max({double(p[0]),double(p[1]),double(p[2])});
    const double low=std::min({double(p[0]),double(p[1]),double(p[2])});
    const double chroma=(peak-low)/(peak+.02);
    // Reduce intervention in noisy blacks, specular highlights and highly
    // saturated illumination. No average-color neutralization or framewise WB.
    double boundary=1;
    for(int c=0;c<3;++c)boundary=std::min(boundary,smooth(0,.025,std::min({double(p[c]),1-double(p[c]),double(ordinary[c]),1-double(ordinary[c])})));
    const double confidence=boundary*smooth(.005,.04,y)*(1-smooth(.55,.95,peak))*(1-.7*smooth(.45,.9,chroma));
    const double shadow=1-smooth(.04,.35,y),mid=1-std::abs(2*smooth(.02,.65,y)-1);
    // Shared creative direction: warm skin/mids, cool sky/water/shadows.
    // Hair/clothing remain restrained; their existing color is not replaced.
    const double warm[]{.030,.002,.003,-.012,-.004,-.010,.002,0};
    const double saturation[]{.015,-.025,-.010,-.015,-.050,-.015,-.020,0};
    double warmth=0,sat=0;
    for(std::size_t i=0;i<w.size();++i){warmth+=w[i]*warm[i];sat+=w[i]*saturation[i];}
    warmth*=.35+.65*mid; // preserve a distinct response across brightness
    warmth-=.003*shadow*(w[3]+w[5]+w[6]);
    const double oy=.2126*ordinary[0]+.7152*ordinary[1]+.0722*ordinary[2];
    double delta[]{warmth+sat*(ordinary[0]-oy),sat*(ordinary[1]-oy),-warmth+sat*(ordinary[2]-oy)};
    const double dy=.2126*delta[0]+.7152*delta[1]+.0722*delta[2];
    double limit=1;
    for(int c=0;c<3;++c){delta[c]=(delta[c]-dy)*confidence*strength_;if(delta[c]>0)limit=std::min(limit,(1-ordinary[c])/delta[c]);else if(delta[c]<0)limit=std::min(limit,-ordinary[c]/delta[c]);}
    // One common scale preserves hue direction and LUT luminance at gamut edge.
    auto out=ordinary;for(int c=0;c<3;++c)out[c]=static_cast<float>(ordinary[c]+std::clamp(limit,0.,1.)*delta[c]);return out;
}
}
