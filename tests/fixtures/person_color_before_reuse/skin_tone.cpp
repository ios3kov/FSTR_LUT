// Frozen 5f2ae8c numerical/error oracle; namespace-only isolation.
#include "skin_tone.hpp"
#include <algorithm>
#include <cmath>
namespace fstr::frozen { namespace {
double smooth(double a,double b,double v){const double t=std::clamp((v-a)/(b-a),0.,1.);return t*t*(3-2*t);}
double luminance(RGB v){return .2126*v.r+.7152*v.g+.0722*v.b;}
void unit(double v){if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Skin confidence requires [0,1]");}
bool usable(RGB v){return std::isfinite(v.r)&&std::isfinite(v.g)&&std::isfinite(v.b)&&std::min({v.r,v.g,v.b})>0&&std::max({v.r,v.g,v.b})<1;}
ColorSettings settings(std::shared_ptr<const LUT> l,double s){ColorSettings v;v.look=std::move(l);v.look_amount=s;return v;}
double median(std::vector<std::pair<double,double>> v){std::sort(v.begin(),v.end());double mass=0;for(auto p:v)mass+=p.second;double cumulative=0;for(auto p:v){cumulative+=p.second;if(cumulative>=mass*.5)return p.first;}return 0;}
}
SkinToneLook::SkinToneLook(std::shared_ptr<const LUT> l,double s):ordinary_(settings(std::move(l),s),Transfer::Linear,Primaries::SRGB_Rec709,Alpha::Straight){}
SkinToneReference SkinToneLook::reference(const std::vector<SkinSample>& samples)const{
    if(samples.size()>1048576)throw std::invalid_argument("Skin sample budget exceeded");
    std::vector<std::pair<double,double>> channels[3];double mass=0;std::size_t count=0;
    for(const auto&s:samples){unit(s.confidence);if(!std::isfinite(s.color.r)||!std::isfinite(s.color.g)||!std::isfinite(s.color.b))throw std::invalid_argument("Nonfinite skin sample");if(!usable(s.color))continue;const double y=luminance(s.color);
        // Numerical near-black guard only; no light-skin brightness target.
        if(y<1e-6||s.confidence==0)continue;
        auto q=ordinary_.apply({float(s.color.r),float(s.color.g),float(s.color.b),1});
        if(!usable({q[0],q[1],q[2]}))continue;
        const double qy=luminance({q[0],q[1],q[2]});if(qy<1e-6)continue;
        const double source[]{s.color.r,s.color.g,s.color.b};
        for(std::size_t c=0;c<3;++c)channels[c].push_back({q[c]/qy-source[c]/y,s.confidence});
        mass+=s.confidence;++count;
    }
    SkinToneReference r;r.binding_=binding_;r.samples=count;if(count<8)return r;
    r.chroma_delta={median(channels[0]),median(channels[1]),median(channels[2])};
    const double dy=luminance(r.chroma_delta);r.chroma_delta.r-=dy;r.chroma_delta.g-=dy;r.chroma_delta.b-=dy;
    // Bounded intervention; a strongly stylized LUT still supplies its direction.
    const double peak=std::max({std::abs(r.chroma_delta.r),std::abs(r.chroma_delta.g),std::abs(r.chroma_delta.b)});
    if(peak>.5){r.chroma_delta.r*=.5/peak;r.chroma_delta.g*=.5/peak;r.chroma_delta.b*=.5/peak;}
    r.confidence=(mass/double(count))*smooth(7.,32.,double(count));return r;
}
std::array<float,4> SkinToneLook::apply(std::array<float,4> p,const SkinToneReference&r,double mask,bool enabled)const{
    return apply_from_ordinary(p,ordinary_.apply(p),r,mask,enabled);
}
std::array<float,4> SkinToneLook::apply_from_ordinary(std::array<float,4> p,std::array<float,4> q,const SkinToneReference&r,double mask,bool enabled)const{
    if(!enabled||ordinary_.bypass()||p[3]==0)return q;
    unit(mask);unit(r.confidence);
    if(r.confidence>0&&r.binding_!=binding_)throw std::invalid_argument("Skin reference belongs to a different look/strength context");
    if(!std::isfinite(r.chroma_delta.r)||!std::isfinite(r.chroma_delta.g)||!std::isfinite(r.chroma_delta.b)||
       std::max({std::abs(r.chroma_delta.r),std::abs(r.chroma_delta.g),std::abs(r.chroma_delta.b)})>.500001||std::abs(luminance(r.chroma_delta))>1e-6)
        throw std::invalid_argument("Invalid skin reference");
    double boundary=1;for(std::size_t c=0;c<3;++c)boundary=std::min(boundary,smooth(0,.001,std::min({double(p[c]),1.-p[c],double(q[c]),1.-q[c]})));
    double weight=mask*r.confidence*boundary;if(weight==0||!usable({p[0],p[1],p[2]})||!usable({q[0],q[1],q[2]}))return q;
    const double y=luminance({p[0],p[1],p[2]}),qy=luminance({q[0],q[1],q[2]});weight*=smooth(1e-6,1e-5,y);if(weight==0)return q;
    const double change[]{r.chroma_delta.r,r.chroma_delta.g,r.chroma_delta.b};double delta[3];
    for(std::size_t c=0;c<3;++c)delta[c]=(qy*(p[c]/y+change[c])-q[c])*weight;
    const double dy=.2126*delta[0]+.7152*delta[1]+.0722*delta[2];double limit=1;
    for(std::size_t c=0;c<3;++c){delta[c]-=dy;if(delta[c]>0)limit=std::min(limit,(1-q[c])/delta[c]);else if(delta[c]<0)limit=std::min(limit,-q[c]/delta[c]);}
    for(std::size_t c=0;c<3;++c)q[c]=float(q[c]+limit*delta[c]);return q;
}
std::array<float,4> SkinToneLook::apply_people(std::array<float,4> p,const std::vector<SkinToneInfluence>& people,bool enabled)const{
    return people_from_ordinary(p,ordinary_.apply(p),people,enabled);
}
std::array<float,4> SkinToneLook::people_from_ordinary(std::array<float,4> p,std::array<float,4> base,const std::vector<SkinToneInfluence>& people,bool enabled)const{
    if(!enabled||ordinary_.bypass()||p[3]==0)return base;
    if(people.size()>16)throw std::invalid_argument("Person field budget exceeded");
    double mass=0;for(const auto&v:people){unit(v.weight);mass+=v.weight;if(!v.reference)throw std::invalid_argument("Missing person reference");}
    if(mass>1.000001)throw std::invalid_argument("Person field ownership exceeds one");
    // Convex combination of complete results. Fractional overlap never grades twice.
    double values[]{base[0],base[1],base[2]};
    const double normalize=std::max(1.,mass);
    for(const auto&v:people){const auto next=apply_from_ordinary(p,base,*v.reference,1,true);for(std::size_t c=0;c<3;++c)values[c]+=(v.weight/normalize)*(double(next[c])-base[c]);}
    auto result=base;for(std::size_t c=0;c<3;++c)result[c]=float(values[c]);return result;
}

}
