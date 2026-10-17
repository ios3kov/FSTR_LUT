#include "fstr/scene_color.hpp"
#include <fstream>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <sstream>
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("LUT path");std::ifstream f(argv[1]);auto lut=std::make_shared<fstr::LUT>(fstr::LUT::parse(f));
 for(double amount:{0.,1.,3.}){fstr::SceneLook scene(lut,amount);fstr::ColorSettings s;s.look=lut;s.look_amount=amount;fstr::ColorProcessor ordinary(s,fstr::Transfer::Linear,fstr::Primaries::SRGB_Rec709,fstr::Alpha::Straight);
 for(auto p:{std::array<float,4>{.2f,.15f,.1f,1},std::array<float,4>{.5f,.3f,.2f,.3f},std::array<float,4>{-.2f,2,.4f,0},std::array<float,4>{-.2f,2,.4f,1}}){fstr::SceneWeights empty{},skin{};skin[0]=1;auto base=ordinary.apply(p),off=scene.apply(p,skin,false),unknown=scene.apply(p,empty),on=scene.apply(p,skin);check(base==off&&base==unknown,"Off/unknown changed");check(on[3]==p[3],"Alpha changed");double dy=0;for(int c=0;c<3;++c)dy+=(on[c]-base[c])*std::array<double,3>{.2126,.7152,.0722}[c];check(std::abs(dy)<1e-7,"Regional luminance seam");if(amount==0||p[3]==0)check(on==p,"Bypass changed");}
 }
 fstr::SceneLook scene(lut,1);fstr::SceneWeights skin{},sky{};skin[0]=1;sky[3]=1;std::array<float,4> p{.2f,.15f,.1f,1};auto a=scene.apply(p,skin),b=scene.apply(p,sky);check(a[0]>b[0]&&a[2]<b[2],"Semantic roles have no distinct tint");
 for(float edge:{0.f,1.f}) {std::array<float,4> p0{edge,.2f,.2f,1},p1{edge+(edge==0?-1e-7f:1e-7f),.2f,.2f,1};auto a=scene.apply(p0,skin),b=scene.apply(p1,skin);for(int c=0;c<3;++c)check(std::abs(a[c]-b[c])<1e-5,"SDR boundary discontinuity");}
 std::istringstream identity("LUT_3D_SIZE 2\n0 0 0\n1 0 0\n0 1 0\n1 1 0\n0 0 1\n1 0 1\n0 1 1\n1 1 1\n");
 fstr::SceneLook identity_scene(std::make_shared<fstr::LUT>(fstr::LUT::parse(identity)),1);
 auto last=identity_scene.apply({-.001f,.2f,.2f,1},skin);
 for(int i=0;i<=300;++i){auto v=identity_scene.apply({-.001f+i*.00001f,.2f,.2f,1},skin);for(int c=0;c<3;++c)check(std::abs(v[c]-last[c])<.0001,"Identity gamut-edge discontinuity");last=v;}
 fstr::SceneWeights invalid{};invalid[0]=invalid[1]=.6f;bool caught=false;try{scene.apply(p,invalid);}catch(...){caught=true;}check(caught,"Overlapping mass accepted");
 for(int i=0;i<100;++i){p={i/100.f,i/110.f,i/120.f,1};auto v=scene.apply(p,skin);for(int c=0;c<3;++c)check(std::isfinite(v[c])&&v[c]>=0&&v[c]<=1,"SDR correction clips");}
 auto reference=scene.skin_reference(std::vector<fstr::SkinSample>(40,{{.23,.12,.07},1}));
 fstr::SkinToneLook independent(lut,1);auto independent_ref=independent.reference(std::vector<fstr::SkinSample>(40,{{.23,.12,.07},1}));
 p={.11f,.055f,.035f,1};check(scene.apply(p,skin,true,&reference)==independent.apply(p,independent_ref,1),"Scene does not use individual skin path");
 check(scene.apply(p,sky,true,&reference)==scene.apply(p,sky),"Person reference affects non-skin");
 fstr::SkinToneReference unknown_person;fstr::ColorSettings ordinary_settings;ordinary_settings.look=lut;fstr::ColorProcessor plain(ordinary_settings,fstr::Transfer::Linear,fstr::Primaries::SRGB_Rec709,fstr::Alpha::Straight);
 check(scene.apply(p,skin,true,&unknown_person)==plain.apply(p),"Unknown person must use ordinary LUT");
 for(int i=0;i<=100;++i){fstr::SceneWeights mixed{};mixed[0]=float(i)/100;mixed[3]=1-mixed[0];auto v=scene.apply(p,mixed,true,&reference),b=plain.apply(p);double dy=0;for(int c=0;c<3;++c){check(std::isfinite(v[c])&&v[c]>=0&&v[c]<=1,"Mixed skin/scene gamut");dy+=(v[c]-b[c])*std::array<double,3>{.2126,.7152,.0722}[c];}check(std::abs(dy)<1e-7,"Mixed roles changed LUT luminance");}
 std::cout<<"PASS scene unknown/off, roles, luminance, alpha, range and overlap contracts\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
