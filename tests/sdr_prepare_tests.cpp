#include "fstr/sdr_prepare.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <future>
#include <iostream>
#include <limits>
using namespace fstr;
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
void near(double a,double b,double e=1e-10){check(std::abs(a-b)<=e,"Numeric preparation oracle");}
FrameView view(const std::vector<float>& v,std::size_t w){return {v.data(),v.size(),w,1,w*4,Transfer::Linear};}
std::vector<float> ramp(double low,double high){std::vector<float> v;for(int i=0;i<96;++i){float x=float(low+(high-low)*i/95);v.insert(v.end(),{x,x,x,1});}return v;}
int main(){try{
    auto flat=ramp(.08,.4),rich=ramp(.001,.95),dark=ramp(.001,.05),bright=ramp(.6,.95),constant=ramp(.2,.2);
    const auto estimated=analyze_sdr_shot({view(flat,96),view(flat,96)});
    check(estimated.decision==SdrDecision::Prepared&&estimated.settings.contrast>1&&estimated.samples==192,"Flat shot was not prepared");
    auto milky=ramp(.04,.4);const auto recovery=analyze_sdr_shot({view(milky,96)});
    check(prepare_sdr_linear({.4,.4,.4},recovery.settings).r>.6,"Flat white anchor remained gray");
    check(prepare_sdr_linear({.04,.04,.04},recovery.settings).r<.04,"Lifted black anchor did not deepen");
    check(analyze_sdr_shot({view(rich,96)}).decision==SdrDecision::Balanced,"Already wide-range shot changed");
    for(const auto& v:{dark,bright,constant})check(analyze_sdr_shot({view(v,96)}).decision==SdrDecision::Ambiguous,"Ambiguous shot forced into target");
    check(analyze_sdr_shot({view(flat,96),view(rich,96)}).decision==SdrDecision::Inconsistent,"Mixed anchors accepted");
    auto flat2=ramp(.09,.41);auto a=analyze_sdr_shot({view(flat,96),view(flat2,96)}),b=analyze_sdr_shot({view(flat2,96),view(flat,96)});
    near(a.settings.contrast,b.settings.contrast,0);near(a.settings.brightness_bias,b.settings.brightness_bias,0);near(a.settings.saturation,b.settings.saturation,0);
    SdrPreparation s{1.4,0,1.12,1};auto p=prepare_sdr_linear({.18,.18,.18},s);near(p.r,.18);near(p.g,.18);
    s={1,std::log(1.5),1,1};p=prepare_sdr_linear({.25,.25,.25},s);near(p.r,1./3); // odds1/3 *1.5 =1/2 => output1/3.
    s={1.8,.45,1.12,1};double previous=-1;
    for(int i=0;i<=1000;++i){double x=i/1000.;p=prepare_sdr_linear({x,x,x},s);check(p.r>=previous&&p.r>=0&&p.r<=1,"Gray curve lost monotonicity/endpoints");previous=p.r;}
    p=prepare_sdr_linear({1e-20,1e-20,1e-20},s);check(p.r>0&&p.r<1e-15,"Artificial black floor");
    // Dense saturated cube: gamut bound AND luminance agree with an independently
    // evaluated scalar curve. A hard RGB clamp would violate luminance here.
    for(int r=0;r<=12;++r)for(int g=0;g<=12;++g)for(int bl=0;bl<=12;++bl){
        RGB in{r/12.,g/12.,bl/12.};p=prepare_sdr_linear(in,s);for(double x:{p.r,p.g,p.b})check(x>=0&&x<=1,"SDR cube escaped");
        double y=.2126*in.r+.7152*in.g+.0722*in.b;
        if(y>0&&y<1){double odds=std::pow((y/(1-y))/(.18/.82),1.8)*(.18/.82)*std::exp(.45);near(.2126*p.r+.7152*p.g+.0722*p.b,odds/(1+odds));}
    }
    auto v=flat;v[3]=0;v[0]=std::numeric_limits<float>::quiet_NaN();s.amount=0;auto out=render_sdr_preparation(view(v,96),s);check(out.status==Status::Ok&&std::memcmp(v.data(),out.rgba.data(),v.size()*4)==0,"Amount0 changed object bytes");
    s.amount=1;out=render_sdr_preparation(view(v,96),s);check(out.status==Status::Ok&&std::memcmp(v.data(),out.rgba.data(),16)==0,"Hidden transparent RGB changed");
    std::vector<float> pm{.1f,.05f,.025f,.5f,99,99,99,99};FrameView f{pm.data(),pm.size(),1,1,8,Transfer::Linear,Primaries::SRGB_Rec709,Alpha::Premultiplied};out=render_sdr_preparation(f,s);p=prepare_sdr_linear({.2,.1,.05},s);check(out.status==Status::Ok,"Premult render failed");near(out.rgba[0],p.r*.5,2e-8);near(out.rgba[3],.5,0);check(out.rgba.size()==4,"Padding leaked");
    auto bad=flat;bad[0]=2;bool rejected=false;try{analyze_sdr_shot({view(bad,96)});}catch(const std::invalid_argument&){rejected=true;}check(rejected,"HDR treated as SDR");
    rejected=false;try{analyze_sdr_shot({view(constant,96),view(bad,96)});}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Ambiguous first anchor hid invalid later data");
    for(auto transfer:{Transfer::SRGB,Transfer::Gamma24}){
        float code=float(transfer==Transfer::SRGB?linear_to_srgb(.25):std::pow(.25,1/2.4));std::vector<float> gray{code,code,code,1};auto gf=view(gray,1);gf.transfer=transfer;
        auto got=render_sdr_preparation(gf,{1,std::log(1.5),1,1});check(got.status==Status::Ok,"Transfer render failed");near(got.rgba[0],transfer==Transfer::SRGB?linear_to_srgb(1./3):std::pow(1./3,1/2.4),1e-7);
    }
    f=view(flat,96);f.count=3;check(render_sdr_preparation(f,s).status==Status::InvalidInput,"Bad extent accepted");f=view(flat,96);f.memory=Memory::GPU;check(render_sdr_preparation(f,s).status==Status::Unsupported,"GPU dereferenced");
    std::atomic_bool stop{true};ColorContext context;context.cancelled=&stop;check(render_sdr_preparation(view(flat,96),s,context).status==Status::Cancelled,"Cancellation ignored");
    rejected=false;try{analyze_sdr_shot({view(flat,96)},context);}catch(const std::runtime_error&){rejected=true;}check(rejected,"Cancelled analysis accepted");
    auto render=[&]{return render_sdr_preparation(view(flat,96),estimated.settings).rgba;};auto x=std::async(std::launch::async,render),z=std::async(std::launch::async,render);check(x.get()==z.get(),"Concurrent render changed result");
    rejected=false;try{analyze_sdr_shot({});}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Empty anchors accepted");
    std::cout<<"PASS SDR preparation: bounded shot estimation, ambiguity/cut guard, order/thread invariance, scalar and gamut/luminance oracles, endpoints, alpha/premult/stride, HDR rejection, cancellation\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
