#include "fstr/scene_color.hpp"
#include <atomic>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fstr;
void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
template<class F>void rejects(F f,const char*message){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}check(rejected,message);}
int main(){try{
 std::atomic<int> calls{0};auto lut=std::make_shared<LUT>(LUT::from_sampler([&](RGB p){++calls;return RGB{.94*p.r+.02,.92*p.g+.03,.91*p.b+.04};}));
 const std::array<float,4> p{.2f,.15f,.1f,1};SceneWeights w{};w[0]=.6f;w[3]=.4f;SceneLook scene(lut,.65);
 auto a=scene.skin_reference(std::vector<SkinSample>(40,{{.1,.06,.03},1})),b=scene.skin_reference(std::vector<SkinSample>(40,{{.5,.3,.2},1}));
 std::vector<SkinToneInfluence> people{{&a,.3},{&b,.3}};std::vector<StableToneWeight> stable{{.3,.7},{.4,.9}};
 calls=0;const auto residual=scene.stable_residual(p,w,people,stable);check(calls==1,"Smart residual repeats ordinary LUT evaluation");
 calls=0;const auto styled=scene.apply_stable_people(p,w,people,stable);check(calls==1,"Stable person path repeats ordinary LUT");
 ColorSettings settings;settings.look=lut;settings.look_amount=.65;ColorProcessor plain(settings,Transfer::Linear,Primaries::SRGB_Rec709,Alpha::Straight);const auto ordinary=plain.apply(p);
 for(std::size_t c=0;c<3;++c)check(residual[c]==double(styled[c])-ordinary[c],"Residual changed float-to-double subtraction");
 calls=0;scene.apply_people(p,w,people);check(calls==1,"Scene people path repeats ordinary LUT");calls=0;scene.apply(p,w,true,&a);check(calls==1,"Single scene person repeats ordinary LUT");
 SkinToneLook skin(lut,.65);auto own=skin.reference(std::vector<SkinSample>(40,{{.2,.1,.08},1}));calls=0;skin.apply_people(p,{{&own,.3},{&own,.4}});check(calls==1,"Skin mixture repeats ordinary LUT");
 // Zero-weight records still validate their references, before stable shortcuts.
 SceneWeights empty{};rejects([&]{scene.apply_stable_people(p,empty,{{nullptr,0}},{});},"Stable empty skipped null zero-weight reference");
 SceneLook foreign(lut,.65);auto stale=foreign.skin_reference(std::vector<SkinSample>(40,{{.2,.1,.08},1}));rejects([&]{scene.stable_residual(p,empty,{{&stale,0}},{});},"Zero-weight foreign binding accepted");rejects([&]{scene.stable_residual(p,empty,{{&stale,0}},{{0,0}});},"Zero support skipped foreign binding");
 auto invalid=w;invalid[0]=std::numeric_limits<float>::quiet_NaN();const auto off=scene.stable_residual(p,invalid,{{nullptr,1}},{{-1,2}},false);check(off==std::array<double,3>{},"Off consumed semantic fields");
 auto extended=p;extended[0]=-.1f;check(scene.apply(extended,w,true,&stale)==plain.apply(extended),"Extended SDR fallback inspected foreign reference");
 rejects([&]{scene.stable_residual(p,w,people,{{1,1}});},"Stable person mismatch accepted");
 std::vector<SkinToneInfluence> crowd(16,{&a,double(w[0])/16});std::vector<StableToneWeight> crowd_stable(16,{1./16,.5});calls=0;scene.stable_residual(p,w,crowd,crowd_stable);check(calls==1,"Crowd repeats ordinary LUT");
 SceneLook zero(lut,0);calls=0;zero.stable_residual(p,invalid,{{nullptr,2}},{{-1,2}});check(calls==0,"Zero look strength evaluates LUT");
 std::cout<<"PASS single immutable LUT evaluation, residual precision, validation order, bindings, crowd and bypass\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
