#include "fstr/skin_tone.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
double y(const std::array<float,4>&p){return .2126*p[0]+.7152*p[1]+.0722*p[2];}
int main(int argc,char**argv){try{
 check(argc==2,"LUT required");std::ifstream f(argv[1]);auto lut=std::make_shared<fstr::LUT>(fstr::LUT::parse(f));
 fstr::SkinToneLook look(lut,1);fstr::ColorSettings settings;settings.look=lut;
 fstr::ColorProcessor base(settings,fstr::Transfer::Linear,fstr::Primaries::SRGB_Rec709,fstr::Alpha::Straight);
 std::vector<fstr::SkinToneReference> refs;
 for(fstr::RGB skin: {fstr::RGB{.065,.032,.019},fstr::RGB{.23,.12,.07},fstr::RGB{.57,.38,.25}}){
  std::vector<fstr::SkinSample> samples(40,{skin,1});auto r=look.reference(samples);check(r.samples==40&&r.confidence==1,"Valid tone reference missing");
  samples.push_back({{.01,.8,.01},.5});auto robust=look.reference(samples);check(std::abs(robust.chroma_delta.r-r.chroma_delta.r)<1e-9,"Outlier moved reference");
  std::reverse(samples.begin(),samples.end());auto reversed=look.reference(samples);check(reversed.chroma_delta.r==robust.chroma_delta.r,"Sample order changed reference");refs.push_back(r);
  for(double shade:{.25,.5,1.}){
   std::array<float,4> p{float(skin.r*shade),float(skin.g*shade),float(skin.b*shade),.6f};auto ordinary=base.apply(p),out=look.apply(p,r,1);
   check(out[3]==p[3]&&std::abs(y(out)-y(ordinary))<1e-7,"Skin adaptation changed alpha/luminance");
   check(look.apply(p,r,0)==ordinary&&look.apply(p,r,1,false)==ordinary,"Off/zero mask differs");
   for(int c=0;c<3;++c)check(std::isfinite(out[c])&&out[c]>=0&&out[c]<=1,"Invalid gamut");
  }
 }
 check(std::abs(refs.front().chroma_delta.r-refs.back().chroma_delta.r)>1e-4,"Different tones share one reference");
 check(look.reference(std::vector<fstr::SkinSample>(7,{{.2,.1,.08},1})).confidence==0,"Sparse evidence accepted");
 std::vector<fstr::SkinSample> invalid{{{.2,.1,.08},2}};bool caught=false;try{look.reference(invalid);}catch(const std::invalid_argument&){caught=true;}check(caught,"Invalid confidence accepted");
 fstr::SkinToneReference empty;std::array<float,4> p{.2,.1,.08,1};check(look.apply(p,empty,1)==base.apply(p),"Unknown reference altered color");
 for(float edge:{0.f,1.f})for(int channel=0;channel<3;++channel){
  auto a=p,b=p;a[channel]=edge-1e-7f;b[channel]=edge+1e-7f;
  auto av=look.apply(a,refs[1],1),bv=look.apply(b,refs[1],1);
  for(int c=0;c<3;++c)check(std::abs(av[c]-bv[c])<1e-5,"Gamut boundary discontinuity");
 }
 for(float level:{.999e-6f,1.001e-6f}){std::array<float,4> dark{level,level,level,1};auto v=look.apply(dark,refs[1],1);auto b=base.apply(dark);for(int c=0;c<3;++c)check(std::abs(v[c]-b[c])<1e-6,"Near-black threshold discontinuity");}
 fstr::SkinToneLook other(lut,.5);caught=false;try{other.apply(p,refs[0],1);}catch(const std::invalid_argument&){caught=true;}check(caught,"Stale LUT/strength reference accepted");
 auto identity=std::make_shared<fstr::LUT>(fstr::LUT::identity(2));fstr::SkinToneLook identity_look(identity,1);
 auto identity_ref=identity_look.reference(std::vector<fstr::SkinSample>(40,{{.2,.1,.08},1}));
 auto identity_out=identity_look.apply(p,identity_ref,1);for(int c=0;c<3;++c)check(std::abs(identity_out[c]-p[c])<1e-6,"Identity LUT creates skin cast");
 for(auto extended:{std::array<float,4>{-.1,.2,.3,1},std::array<float,4>{1.1,.2,.3,1}})check(look.apply(extended,refs[0],1)==base.apply(extended),"Extended samples changed");
 auto invalid_reference=refs[0];invalid_reference.chroma_delta.r=2;caught=false;try{look.apply(p,invalid_reference,1);}catch(const std::invalid_argument&){caught=true;}check(caught,"Invalid reference accepted");
 fstr::SkinToneLook natural(nullptr,1);auto r=natural.reference(std::vector<fstr::SkinSample>(40,{{.2,.1,.08},1}));check(natural.apply(p,r,1)==p,"Natural changed skin");
 std::cout<<"PASS individual tones, robust/order-independent references, luminance, alpha, fallback and Natural\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
