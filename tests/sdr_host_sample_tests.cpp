#include "color_pass.hpp"
#include "fstr/sdr_timeline.hpp"
#include <cstring>
#include <iostream>
#include <limits>
using namespace fstr;
void check(bool p,const char*m){if(!p)throw std::runtime_error(m);}
template<class F>void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Out-of-contract value accepted");}
int main(){try{
 constexpr double e=1./32768.;
 for(double x:{-e,0.,-0.,.5,1.,1+e}){auto y=ae::bounded_host_sdr(x);check(y==std::clamp(x,0.,1.),"Boundary normalization");if(x>=0&&x<=1)check(!std::memcmp(&x,&y,sizeof(x)),"Valid bits changed");}
 for(double x:{std::nextafter(-e,-1.),std::nextafter(1+e,2.),-.01,1.01,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})rejects([&]{ae::bounded_host_sdr(x);});
 SdrPreparation recipe;recipe.contrast=1.31;recipe.brightness_bias=.08;recipe.saturation=1.04;
 ColorSettings settings;settings.mix=1;
 for(auto transfer:{Transfer::SRGB,Transfer::Linear,Transfer::Gamma24})for(float alpha:{.001f,.5f,1.f}){
  ae::ColorPass pass(settings,transfer,Primaries::SRGB_Rec709,&recipe);
  // Exact observed PNG white; binary-friendly premultiplication endpoints too.
  auto raw=std::array<float,4>{.9765007495880127f*alpha,.99612414836883545f*alpha,float(1+e)*alpha,alpha};
  // Sub-half alpha can amplify float rounding slightly past the exact code;
  // use exact representable boundary there, without widening tolerance.
  if(alpha<.5f)raw[2]=alpha;
  auto bounded=raw;bounded[2]=alpha;
  auto a=pass.apply(raw),b=pass.apply(bounded);check(!std::memcmp(a.data(),b.data(),sizeof(a)),"Observed pixel differs from bounded white oracle");
  auto bad=raw;bad[2]=1.01f*alpha;rejects([&]{pass.apply(bad);});
  bad[2]=std::numeric_limits<float>::quiet_NaN();rejects([&]{pass.apply(bad);});
 }
 float anchor[]{.97650075f,.99612415f,float(1+e),1};ae::normalize_sdr_anchor(anchor);check(anchor[2]==1,"Anchor residue remains");
 FrameView view{anchor,4,1,1,4,Transfer::SRGB,Primaries::SRGB_Rec709,Alpha::Premultiplied};describe_sdr_cut_frame(view);
 anchor[2]=float(1+e);rejects([&]{describe_sdr_cut_frame(view);});rejects([&]{prepare_sdr_linear({.5,.5,1+e},recipe);});
 float hidden[]{2,-2,3,.1f};auto original=std::array<float,4>{2,-2,3,.1f};ae::normalize_sdr_anchor(hidden);check(!std::memcmp(hidden,original.data(),sizeof(hidden)),"Excluded anchor modified");
 // Disabled preparation and Mix=0 keep general extended-float behaviour.
 auto raw=std::array<float,4>{.4f,.3f,1.01f,1};ae::ColorPass off(settings,Transfer::SRGB,Primaries::SRGB_Rec709,nullptr);check(off.apply(raw)==raw,"Ordinary extended bypass changed");settings.mix=0;ae::ColorPass mix0(settings,Transfer::SRGB,Primaries::SRGB_Rec709,&recipe);check(mix0.apply(raw)==raw,"Mix0 changed");
 std::cout<<"PASS exact AE residue, strict endpoints/nonfinite,3 transfers, premultiplied alpha, unchanged core and bypass\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
