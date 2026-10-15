#include "fstr/smart_color.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <future>
#include <sstream>
using namespace fstr;
int checks=0;
void require(bool ok){++checks;if(!ok)throw std::runtime_error("Smart contract failed #"+std::to_string(checks));}
double y(RGB v){return .2126*v.r+.7152*v.g+.0722*v.b;}
bool near(double a,double b,double eps=1e-12){return std::abs(a-b)<=eps;}
template<class F>void bad(F f){bool threw=false;try{f();}catch(const std::invalid_argument&){threw=true;}require(threw);}
int main(){try{
    // Independent oracle: removing the delta's chroma retains styled luma;
    // full protection makes every RGB delta equal the same scalar luminance delta.
    for(RGB base: {RGB{.3,.2,.1},RGB{0,0,0},RGB{-.1,2,.2},RGB{1e-8,2e-8,0}})
    for(RGB styled: {RGB{.1,.3,.4},RGB{-.2,1.2,.4},RGB{0,0,0}})
    for(double p:{0.,.25,1.}){
        auto out=protect_skin_chroma(base,styled,p,1);
        require(near(y(out),y(styled)));
        if(p==0)require(out.r==styled.r&&out.g==styled.g&&out.b==styled.b);
        if(p==1)require(near(out.r-base.r,out.g-base.g)&&near(out.r-base.r,out.b-base.b));
    }
    const auto dark=protect_skin_chroma({.8,.2,.1},{.01,.01,.01},1,1);
    require(dark.b<0);require(near(y(dark),.01)); // No hidden gamut clamp.
    const RGB seed{-.0,.18,2};auto same=prepare_smart_color(seed,{});
    require(std::signbit(same.r)&&same.g==seed.g&&same.b==seed.b);
    auto gain=prepare_smart_color({.2,.4,.8},{.5,{1,1,1}});
    require(near(gain.r,.2*std::sqrt(2.))&&near(gain.b,.8*std::sqrt(2.)));
    std::istringstream cube("LUT_3D_SIZE 2\n0 0 0\n0 0 1\n1 0 0\n1 0 1\n0 1 0\n0 1 1\n1 1 0\n1 1 1\n");
    auto lut=std::make_shared<LUT>(LUT::parse(cube)); // Nonidentity channel rotation.
    for(double amount:{0.,1.,2.,3.}){
        SmartLook smart(lut,amount,0);ColorSettings old;old.look=lut;old.look_amount=amount;
        ColorProcessor reference(old,Transfer::Linear,Primaries::SRGB_Rec709,Alpha::Straight);
        for(RGB b:{RGB{.25,.5,.75},RGB{-.1,1.1,2.}}){
            // Baseline uses float inputs and output, so compare at native float tolerance.
            b={static_cast<float>(b.r),static_cast<float>(b.g),static_cast<float>(b.b)};
            auto a=smart.apply(b,.8);auto o=reference.apply({static_cast<float>(b.r),static_cast<float>(b.g),static_cast<float>(b.b),1});
            require(near(a.r,o[0],2e-7)&&near(a.g,o[1],2e-7)&&near(a.b,o[2],2e-7));
        }
    }
    for(double invalid:{-1.,1.01,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})bad([&]{protect_skin_chroma({0,0,0},{1,1,1},invalid,1);});
    bad([]{prepare_smart_color({0,0,0},{.51,{1,1,1}});});
    bad([]{prepare_smart_color({0,0,0},{0,{.89,1,1}});});
    bad([]{prepare_smart_color({std::numeric_limits<double>::infinity(),0,0},{});});
    bad([&]{SmartLook invalid(lut,3.01,1);});
    auto no_look=SmartLook(nullptr,3,1).apply(seed,1);
    require(std::signbit(no_look.r)&&no_look.g==seed.g&&no_look.b==seed.b);
    auto zero_look=SmartLook(lut,0,1).apply(seed,.5);
    require(std::signbit(zero_look.r)&&zero_look.g==seed.g&&zero_look.b==seed.b);
    bad([]{prepare_smart_color({std::numeric_limits<float>::max(),0,0},{.5,{1,1,1}});});
    SmartLook shared(lut,2,.7);const auto expected=shared.apply({.4,.2,.1},.8);
    std::vector<std::future<RGB>> jobs;for(int i=0;i<8;++i)jobs.push_back(std::async(std::launch::async,[&]{return shared.apply({.4,.2,.1},.8);}));
    for(auto& job:jobs){auto a=job.get();require(a.r==expected.r&&a.g==expected.g&&a.b==expected.b);}
    std::cout<<"PASS "<<checks<<" independent smart numeric contracts\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
