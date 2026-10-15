// Public LUT/color boundaries; art-direction anchors are independent expectations,
// not a second copy of the recipe. This is not film-match or actual AE evidence.
#include "fstr/color.hpp"
#include "fstr_embedded_look.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
int checks=0;
void check(bool ok,const char* text){++checks;if(!ok)throw std::runtime_error(text);}
double y(fstr::RGB c){return .2126*c.r+.7152*c.g+.0722*c.b;}
std::array<float,4> apply(const std::shared_ptr<const fstr::LUT>& lut,double amount,
                          std::array<float,4> pixel,fstr::Alpha alpha=fstr::Alpha::Straight) {
    fstr::ColorSettings s;s.look=lut;s.look_amount=amount;
    return fstr::ColorProcessor(s,fstr::Transfer::SRGB,fstr::Primaries::SRGB_Rec709,alpha).apply(pixel);
}
}
int main(){try{
    check(fstr::builtin_look_count==17,"Expected Natural, legacy separator and15 styles");
    check(std::strcmp(fstr::embedded_looks[0].title,"Natural")==0&&!fstr::embedded_looks[0].cube,"Natural must not be a hidden grade");
    check(std::strcmp(fstr::embedded_looks[1].title,"(-")==0,"Old Warm must not be selectable");
    check(std::strstr(fstr::builtin_look_menu,"Warm")==nullptr,"Legacy Warm visible in new menu");
    std::array<std::shared_ptr<const fstr::LUT>,fstr::builtin_look_count> luts;
    const std::array<fstr::RGB,9> probes{{{.1,.1,.1},{.25,.25,.25},{.5,.5,.5},{.75,.75,.75},
        {.7,.45,.3},{.2,.5,.7},{.5,.2,.6},{.2,.6,.3},{.9,.8,.7}}};
    for(std::size_t i=1;i<luts.size();++i){
        std::istringstream stream(fstr::embedded_looks[i].cube);
        luts[i]=std::make_shared<const fstr::LUT>(fstr::LUT::parse(stream));
        check(luts[i]->size()==(i==1?17u:33u),"Unexpected baked size or changed legacy LUT");
        double last=-1;
        for(int n=0;n<=256;++n){
            const double gray=static_cast<double>(n)/256;auto c=luts[i]->sample({gray,gray,gray});
            check(y(c)+1e-8>=last,"Gray ramp luminance reversal");last=y(c);
            if(i>1)for(double v:{c.r,c.g,c.b})check(std::isfinite(v)&&v>=-1e-8&&v<=1+1e-8,"New SDR LUT node outside declared domain");
        }
        for(const auto& c:probes){
            std::array<float,4> p{static_cast<float>(c.r),static_cast<float>(c.g),static_cast<float>(c.b),.37f};
            auto zero=apply(luts[i],0,p);check(std::memcmp(zero.data(),p.data(),sizeof(p))==0,"Look0 must copy exactly");
            auto natural=apply(nullptr,3,p);check(std::memcmp(natural.data(),p.data(),sizeof(p))==0,"Natural strength must not alter pixels");
            for(double amount:{.5,1.,2.,3.}){
                auto out=apply(luts[i],amount,p);check(out[3]==p[3],"LUT changed alpha");
                for(float v:out)check(std::isfinite(v),"Amplified LUT nonfinite");
                std::array<float,4> premult{p[0]*p[3],p[1]*p[3],p[2]*p[3],p[3]};
                auto premult_out=apply(luts[i],amount,premult,fstr::Alpha::Premultiplied);
                for(int k=0;k<3;++k)check(std::abs(premult_out[k]-out[k]*p[3])<2e-6,"Premultiplied strength mismatch");
            }
            // Independent extrapolation expectation: linear-light delta doubles.
            auto one=apply(luts[i],1,p),two=apply(luts[i],2,p);
            for(int k=0;k<3;++k){
                double base=fstr::srgb_to_linear(p[k]);
                check(std::abs((fstr::srgb_to_linear(two[k])-base)-2*(fstr::srgb_to_linear(one[k])-base))<2e-6,"Above100 strength silently clamped or wrong space");
            }
        }
    }
    for(std::size_t i=2;i<luts.size();++i)for(std::size_t j=0;j<i;++j){
        if(j==1)continue; // Legacy compatibility is not a new selectable style.
        double delta=0;
        for(auto p:probes){auto a=luts[i]->sample(p),b=j?luts[j]->sample(p):p;
            delta+=std::abs(a.r-b.r)+std::abs(a.g-b.g)+std::abs(a.b-b.b);}
        check(delta/(3*probes.size())>.012,"Selectable looks too similar on declared swatches");
    }
    auto matrix=luts[2]->sample({.5,.5,.5});check(matrix.g>matrix.r+.12&&matrix.g>matrix.b+.12,"Matrix green anchor missing");
    auto desert=luts[3]->sample({.75,.75,.75});check(desert.r>desert.b+.15,"Desert warmth missing");
    auto steel=luts[7]->sample({.25,.25,.25});check(steel.b>steel.r+.1,"Steel cool anchor missing");
    auto neon=luts[9]->sample({.5,.5,.5});check(neon.r>neon.g+.12&&neon.b>neon.g+.12,"Neon magenta anchor missing");
    for(double invalid:{-.001,3.001,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}){
        bool rejected=false;try{apply(luts[2],invalid,{.2f,.3f,.4f,1.f});}catch(const std::invalid_argument&){rejected=true;}
        check(rejected,"Invalid look strength accepted");
    }
    // Domain sampling clamps only the look branch; strength0 retains HDR/negative.
    std::array<float,4> extended{-.2f,1.6f,.4f,0.f};
    auto hidden=apply(luts[2],3,extended,fstr::Alpha::Premultiplied);
    check(std::memcmp(hidden.data(),extended.data(),sizeof(extended))==0,"Hidden RGB changed atalpha0");
    std::cout<<"PASS "<<checks<<" cinema/strength contract assertions; artistic/actual AE acceptance separate\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<" after "<<checks<<" assertions\n";return 1;}}
