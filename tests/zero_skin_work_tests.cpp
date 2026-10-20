#include "fstr/scene_color.hpp"
#include "fixtures/person_color_before_reuse/scene_color.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
using namespace fstr;
struct Result { std::array<double,4> value{}; std::string error; bool operator==(const Result&o)const{return error==o.error&&std::memcmp(value.data(),o.value.data(),sizeof(value))==0;} };

template<class S,class I,class W,class Sample,class Weights> std::vector<Result> run(std::shared_ptr<const LUT> lut,double strength){
 S s(lut,strength),foreign(lut,strength);
 auto a=s.skin_reference(std::vector<Sample>(40,{{.1,.06,.03},1})),b=s.skin_reference(std::vector<Sample>(40,{{.5,.3,.2},1})),stale=foreign.skin_reference(std::vector<Sample>(40,{{.3,.2,.1},1}));
 auto bad=a;bad.chroma_delta.r=std::numeric_limits<double>::quiet_NaN();
 std::vector<Result> out;
 const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
 std::vector<std::array<float,4>> pixels{{.2f,.15f,.1f,1},{0,0,0,1},{1,1,1,1},{-.1f,.2f,1.1f,1},{.2f,.1f,.05f,0},{.2f,.1f,.05f,.4f},{nan,.2f,.1f,1},{inf,.2f,.1f,1},{.1f,.2f,.3f,nan},{.1f,.2f,.3f,-.1f},{.1f,.2f,.3f,1.1f},{-0.f,0.f,-0.f,1}};
 for(int i=0;i<64;++i)pixels.push_back({float(i)/63,float((i*17)%64)/63,float((i*31)%64)/63,1});
 for(auto p:pixels)for(int count: {0,1,2,16,17})for(int mode=0;mode<19;++mode)for(bool enabled:{false,true})for(int method=0;method<3;++method){
  Weights w{};w[0]=.6f;w[3]=.4f;std::vector<I> people(count);std::vector<W> stable(count);
  for(int i=0;i<count;++i){people[i]={i%2?&a:&b,count?.6/count:0};stable[i]={count?.7/count:0,.8};}
  if(mode==0)stable.clear();if(mode==1)for(auto&x:stable)x.support=0;
  if(mode==2)for(auto&x:stable)x.support=1;
  if(mode==3&&!people.empty())people[0].reference=nullptr;
  if(mode==4&&!people.empty()){people[0].reference=&stale;people[0].weight=0;}
  if(mode==5&&!people.empty())people[0].reference=&bad;
  if(mode==6)stable.push_back({0,0});
  if(mode==7&&!stable.empty())stable[0].support=nan;
  if(mode==8&&!stable.empty())stable[0].strength=2;
  if(mode==9)w[0]=nan;
  if(mode==10&&!people.empty())people[0].weight=-.1;
  if(mode==11&&!people.empty())people[0].weight=1;
  if(mode==12)w.fill(0);
  if(mode==13)for(auto&x:people)x.weight=0;
  if(mode==14)for(std::size_t i=0;i<people.size();++i)if(i%2==0)people[i].weight=0;
  if(mode==15)for(auto&x:people)x.weight=-0.;
  if(mode==16&&!people.empty()){people[0].weight=0;people[0].reference=&bad;}
  if(mode==17){for(auto&x:people)x.weight=0;for(auto&x:stable)x.support=0;}
  if(mode==18){for(auto&x:people)x.weight=0;stable.clear();}
  Result r;
  try{if(method==2){auto v=s.stable_residual(p,w,people,stable,enabled);for(int c=0;c<3;++c)r.value[c]=v[c];}else{auto v=method==0?s.apply_people(p,w,people,enabled):s.apply_stable_people(p,w,people,stable,enabled);for(int c=0;c<4;++c)r.value[c]=v[c];}}
  catch(const std::exception&e){r.error=std::string(typeid(e).name())+":"+e.what();}
  out.push_back(r);
 }
 return out;
}
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("look folder required");std::size_t comparisons=0;
 for(const char*name:{"neutral-v1.cube","teal-orange-v1.cube","neon-pink-cyan-v1.cube","navy-gold-v1.cube"})for(double strength:{0.,.65,1.,2.5}){
  std::ifstream f(std::string(argv[1])+"/"+name);auto lut=std::make_shared<LUT>(LUT::parse(f));
  auto old=run<frozen::SceneLook,frozen::SkinToneInfluence,frozen::StableToneWeight,frozen::SkinSample,frozen::SceneWeights>(lut,strength);
  auto now=run<SceneLook,SkinToneInfluence,StableToneWeight,SkinSample,SceneWeights>(lut,strength);
  if(old.size()!=now.size())throw std::runtime_error("oracle size");for(std::size_t i=0;i<now.size();++i){if(!(old[i]==now[i]))throw std::runtime_error(std::string(name)+" mismatch "+std::to_string(i));++comparisons;}
 }
 std::cout<<"PASS "<<comparisons<<" frozen person/stable/residual bit and exact-error comparisons\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
