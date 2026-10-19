#include "color_pass.hpp"
#include "fixtures/color_pass_before_prelook.hpp"
#include <iostream>
#include <cstring>
#include <limits>
using namespace fstr;
struct Result{std::array<float,4> main{},prepared{};std::string error;};
template<class F>Result run(F&&f){try{return f();}catch(const std::exception&e){Result r;r.error=std::string(typeid(e).name())+"/"+e.what();return r;}}
void check(bool value,const char* msg){if(!value)throw std::runtime_error(msg);}
int main(){try{
 std::size_t calls=0;auto lut=std::make_shared<LUT>(LUT::from_sampler([&](RGB p){++calls;return RGB{.8*p.r+.1*p.g+.02,.9*p.g+.04,.7*p.b+.1*p.r+.03};}));
 SdrPreparation neutral,active;active.contrast=1.31;active.brightness_bias=.08;active.saturation=1.04;
 SdrPreparation amount0=active;amount0.amount=0;SdrPreparation invalid=active;invalid.contrast=2;
 const SdrPreparation* recipes[]{nullptr,&neutral,&active,&amount0,&invalid};
 const float alphas[]{0,.001f,.5f,1,std::numeric_limits<float>::quiet_NaN(),-1,1.01f};
 std::size_t compared=0,shared=0;
 for(int source=0;source<source_profile_count;++source)for(auto transfer:{Transfer::SRGB,Transfer::Linear,Transfer::Gamma24})for(double mix:{0.,.37,1.})for(int grade=0;grade<6;++grade)for(auto recipe:recipes){
  ColorSettings s;s.source=SourceProfile(source);s.look=lut;s.look_amount=1.2;s.mix=mix;
  if(grade==1)s.grade.natural_contrast=true;
  if(grade==2)s.grade.exposure_stops=.125;
  if(grade==3){s.grade.contrast=1.2;s.grade.natural_contrast=true;}
  if(grade==4){s.grade.temperature=.1;s.grade.tint=-.1;s.grade.saturation=.9;}
  if(grade==5)s.shoulder=.2;
  auto prep=s;prep.look.reset();prep.look_amount=0;prep.mix=1;
  frozen::BaselineColorPass old(s,transfer,Primaries::SRGB_Rec709,recipe),oldprep(prep,transfer,Primaries::SRGB_Rec709,recipe);
  fstr::ae::ColorPass fresh(s,transfer,Primaries::SRGB_Rec709,recipe),freshprep(prep,transfer,Primaries::SRGB_Rec709,recipe);
  const bool eligible=source==0&&grade_is_neutral(s.grade)&&s.shoulder==0&&mix>0;check(fresh.can_share_prelook()==eligible,"Eligibility mismatch");
  for(float alpha:alphas)for(int i=0;i<20;++i)for(double protection:{0.,.8}){
   std::array<float,4> raw{float(((i*17)%256)/255.)*alpha,float(((i*37)%32769)/32768.)*alpha,float(((i*73)%1000)/1000.)*alpha,alpha};
   if(i==0)raw={-0.f,0,0,alpha};if(i==1)raw={alpha,alpha,alpha,alpha};if(i==2)raw[0]=-.01f;if(i==3)raw[1]=1.01f;if(i==4)raw[2]=std::numeric_limits<float>::quiet_NaN();if(i==5)raw[0]=std::numeric_limits<float>::infinity();
   auto a=run([&]{Result r;r.main=old.apply(raw,protection);r.prepared=oldprep.apply(raw);return r;});
   auto b=run([&]{Result r;r.main=fresh.apply(raw,protection,eligible?&r.prepared:nullptr);if(!eligible)r.prepared=freshprep.apply(raw);return r;});
   if(a.error!=b.error||(!a.error.size()&&(std::memcmp(a.main.data(),b.main.data(),sizeof(a.main))||std::memcmp(a.prepared.data(),b.prepared.data(),sizeof(a.prepared))))){std::cerr<<"Mismatch source="<<source<<" transfer="<<int(transfer)<<" mix="<<mix<<" grade="<<grade<<" alpha="<<alpha<<" i="<<i<<" old="<<a.error<<" new="<<b.error<<'\n';return 1;}
   ++compared;if(eligible)++shared;
  }
  if(!eligible){std::array<float,4> out{};bool rejected=false;try{fresh.apply({.2f,.3f,.4f,1},0,&out);}catch(const std::logic_error&){rejected=true;}check(rejected,"Ineligible capture accepted");}
 }
 // No look and zero look strength still prepare SDR; capture must preserve bypass behavior.
 for(int mode=0;mode<2;++mode)for(auto transfer:{Transfer::SRGB,Transfer::Linear,Transfer::Gamma24})for(auto recipe:recipes)for(float alpha:{0.f,.5f,1.f}){
  ColorSettings s;if(mode==1){s.look=lut;s.look_amount=0;}
  frozen::BaselineColorPass old(s,transfer,Primaries::SRGB_Rec709,recipe);fstr::ae::ColorPass fresh(s,transfer,Primaries::SRGB_Rec709,recipe);
  const std::array<float,4> raw{.2f*alpha,.3f*alpha,.4f*alpha,alpha};
  auto a=run([&]{Result r;r.main=old.apply(raw);r.prepared=old.apply(raw);return r;});
  auto b=run([&]{Result r;r.main=fresh.apply(raw,0,&r.prepared);return r;});
  check(a.error==b.error,"Bypass error mismatch");if(a.error.empty()){check(!std::memcmp(a.main.data(),b.main.data(),sizeof(a.main)),"Bypass main mismatch");check(!std::memcmp(a.prepared.data(),b.prepared.data(),sizeof(a.prepared)),"Bypass prepared mismatch");}++compared;
 }
 // Count only the isolated apply: this must execute exactly one ordinary LUT sample.
 {ColorSettings s;s.look=lut;fstr::ae::ColorPass counted(s,Transfer::SRGB,Primaries::SRGB_Rec709,&active);std::array<float,4> captured{};const auto before=calls;counted.apply({.2f,.3f,.4f,1},0,&captured);check(calls==before+1,"Capture changed LUT sample count");}
 // Ordinary apply must retain invalid-protection checking; capture cannot hide it.
 ColorSettings s;s.look=lut;fstr::ae::ColorPass fresh(s,Transfer::SRGB,Primaries::SRGB_Rec709,&active);std::array<float,4> capture{};bool rejected=false;try{fresh.apply({.2f,.3f,.4f,1},2,&capture);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid protection accepted");
 std::cout<<"PASS "<<compared<<" frozen main/prepared/error comparisons; "<<shared<<" eligible captures; fallback/mix0/hidden-alpha/SDR-range gates\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
