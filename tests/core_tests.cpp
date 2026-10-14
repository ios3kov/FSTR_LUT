#include "fstr/core.hpp"
#include "fstr/color.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <future>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>
#include <cstring>
using namespace fstr;
namespace {
std::atomic<int> checks{0};
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void near(double a,double b,double tolerance=2e-7){check(std::abs(a-b)<=tolerance,"Numeric oracle mismatch");}
template<class F> void rejects(F function){bool rejected=false;try{function();}catch(const std::exception&){rejected=true;}check(rejected,"Malformed input accepted");}
struct Echo final:InferenceBackend {
    std::atomic<int> calls{0};
    std::string identity() const override{return "test-echo-not-a-model";}
    void infer(const PackedTensor& t,std::vector<float>& out,const std::atomic_bool*) override {
        ++calls;out.assign(t.data.begin(),t.data.begin()+static_cast<std::ptrdiff_t>(12*t.half_width*t.half_height));
    }
};
FrameView view(const std::vector<float>& v,std::size_t w,std::size_t h,std::size_t stride=0){
    return {v.data(),v.size(),w,h,stride?stride:w*4};
}
void lut_tests(){
    for(auto size:{17u,33u,65u}){
        auto lut=LUT::identity(size);
        std::mt19937 random(128);std::uniform_real_distribution<double> value(-2,4);
        for(int i=0;i<250;++i){RGB p{value(random),value(random),value(random)};auto q=lut.sample(p);near(p.r,q.r,2e-14);near(p.g,q.g,2e-14);near(p.b,q.b,2e-14);}
    }
    // Nonlinear cube oracle: tetrahedral rg/gb/br equals min pairs, unlike trilinear.
    std::ostringstream cube;cube<<"LUT_3D_SIZE 2\n";
    for(int b=0;b<2;++b)for(int g=0;g<2;++g)for(int r=0;r<2;++r)cube<<r*g<<" "<<g*b<<" "<<b*r<<"\n";
    std::istringstream stream(cube.str());auto lut=LUT::parse(stream);
    std::array<int,3> permutation{0,1,2};
    do{std::array<double,3> p{};p[permutation[0]]=0.8;p[permutation[1]]=0.5;p[permutation[2]]=0.2;
       auto q=lut.sample({p[0],p[1],p[2]});near(q.r,std::min(p[0],p[1]),1e-14);near(q.g,std::min(p[1],p[2]),1e-14);near(q.b,std::min(p[2],p[0]),1e-14);
    }while(std::next_permutation(permutation.begin(),permutation.end()));
    std::ostringstream domain;domain<<"TITLE \"domain\"\nDOMAIN_MIN -1 -2 -3\nDOMAIN_MAX 1 2 3\nLUT_3D_SIZE 2\n";
    for(int b=0;b<2;++b)for(int g=0;g<2;++g)for(int r=0;r<2;++r)domain<<-1+2*r<<" "<<-2+4*g<<" "<<-3+6*b<<"\n";
    std::istringstream ds(domain.str());auto dl=LUT::parse(ds);auto sample=dl.sample({0.25,-0.5,1.5});near(sample.r,0.25);near(sample.g,-0.5);near(sample.b,1.5);
    for(auto bad:{"","LUT_1D_SIZE 2\n","LUT_3D_SIZE 99999999\n","LUT_3D_SIZE -2\n","LUT_3D_SIZE 2\n0 0 0\n","DOMAIN_MIN 0 0 0 junk\n",
                  "LUT_3D_SIZE 2\nNaN 0 0\n","LUT_3D_SIZE 2\n0 0 0 extra\n","TITLE \"a\"\nTITLE \"b\"\n"})
        rejects([&](){std::istringstream s(bad);LUT::parse(s);});
    rejects([&](){lut.sample({std::numeric_limits<double>::infinity(),0,0});});
    // Seeded bounded byte parser fuzz, no assertion of exhaustive coverage.
    std::mt19937 random(8821);
    for(int n=0;n<1000;++n){std::string bytes;for(unsigned i=0,len=random()%128;i<len;++i)bytes.push_back(static_cast<char>(random()%256));
        try{std::istringstream s(bytes);auto l=LUT::parse(s);auto q=l.sample({0.2,0.4,0.6});check(std::isfinite(q.r),"Nonfinite fuzz output");}catch(const std::invalid_argument&){}}
}
void shuffle_tests(){
    std::vector<float> rgba(3*3*4,1);for(std::size_t i=0;i<9;++i)for(std::size_t c=0;c<3;++c)rgba[i*4+c]=static_cast<float>(i*3+c)/27;
    PackedTensor packed;pack_ffdnet(rgba,3,3,0.05f,packed);
    check(packed.half_width==2&&packed.half_height==2,"Odd padding shape");
    for(std::size_t c=0;c<3;++c)for(std::size_t dy=0;dy<2;++dy)for(std::size_t dx=0;dx<2;++dx){
        near(packed.data[(c*4+dy*2+dx)*4],rgba[(dy*3+dx)*4+c]);
        near(packed.data[(c*4+dy*2+dx)*4+3],rgba[(2*3+2)*4+c]);
    }
    for(std::size_t i=48;i<52;++i)near(packed.data[i],0.05);
    std::vector<float> planes(packed.data.begin(),packed.data.begin()+48),roundtrip;
    unpack_ffdnet(planes,3,3,roundtrip);check(roundtrip==rgba,"Shuffle/crop roundtrip");
    planes[3]=std::numeric_limits<float>::quiet_NaN();rejects([&](){unpack_ffdnet(planes,3,3,roundtrip);});
    rejects([&](){pack_ffdnet(rgba,0,3,0.05f,packed);});
    rejects([&](){pack_ffdnet({},2049,1023,0.05f,packed);});
}
void pipeline_tests(){
    auto backend=std::make_shared<Echo>();RenderContext ctx;ctx.backend=backend;
    std::vector<float> rgba{0.1f,0.2f,0.3f,1, 0.6f,0.4f,0.2f,1};
    RenderSettings s;s.enhance=1;auto r=render({view(rgba,2,1),0},s,ctx);
    check(r.status==Status::Ok,"Echo render failed");for(std::size_t i=0;i<rgba.size();++i)near(r.rgba[i],rgba[i]);
    s.look=std::make_shared<LUT>(LUT::identity(17));r=render({view(rgba,2,1),0},s,ctx);
    check(r.status==Status::Ok&&r.restoration_cache_hit&&backend->calls==1,"Look triggered repeated model inference");
    s.look_amount=0.4;s.enhance=0.3;r=render({view(rgba,2,1),0},s,ctx);
    check(r.restoration_cache_hit&&backend->calls==1,"Enhance blend invalidated restoration");
    s.sigma=20.0f/255;r=render({view(rgba,2,1),0},s,ctx);check(!r.restoration_cache_hit&&backend->calls==2,"Sigma failed to invalidate cache");
    auto changed=rgba;changed[0]=0.2f;r=render({view(changed,2,1),0},s,ctx);check(!r.restoration_cache_hit&&backend->calls==3,"Input failed to invalidate cache");
    s.enhance=0;r=render({view(rgba,2,1),0},s,ctx);check(r.status==Status::Ok&&backend->calls==3,"Disabled Enhance called model");
    auto extended=std::vector<float>{-0.2f,2.0f,0.1f,0, 0.12f,0.2f,0.1f,0.5f};
    s.mix=0;s.enhance=1;r=render({view(extended,2,1),0},s,ctx);
    check(r.status==Status::Ok&&std::memcmp(r.rgba.data(),extended.data(),extended.size()*sizeof(float))==0&&backend->calls==3,"Mix0 not exact bypass");
    s.mix=1;r=render({view(extended,2,1),0},s,ctx);check(r.status==Status::Unsupported&&r.rgba.empty(),"Unsupported AI input silently succeeded");
    s.enhance=0;auto pv=view(extended,2,1);pv.alpha=Alpha::Premultiplied;r=render({pv,0},s,ctx);
    check(r.status==Status::Ok,"LUT-only premult failed");for(std::size_t i=0;i<extended.size();++i)near(r.rgba[i],extended[i]);
    s.enhance=1;auto nv=view(rgba,2,1);nv.primaries=Primaries::Unknown;
    check(render({nv,0},s,ctx).status==Status::Unsupported,"Unknown primaries accepted");
    check(render({view(rgba,2,1),1},s,ctx).status==Status::Unsupported,"Unsupported temporal silently accepted");
    auto short_view=view(rgba,2,1);short_view.count=7;
    check(render({short_view,0},s,ctx).status==Status::InvalidInput,"Short buffer accepted");
    short_view=view(rgba,1,2);short_view.row_stride=std::numeric_limits<std::size_t>::max();
    check(render({short_view,0},s,ctx).status==Status::InvalidInput,"Stride overflow accepted");
    auto nan=rgba;nan[0]=std::numeric_limits<float>::quiet_NaN();
    check(render({view(nan,2,1),0},s,ctx).status==Status::InvalidInput,"NaN accepted");
    s.enhance=std::numeric_limits<double>::infinity();
    check(render({view(rgba,2,1),0},s,ctx).status==Status::InvalidInput,"Invalid settings accepted");s.enhance=1;
    std::atomic_bool cancel{true};ctx.cancelled=&cancel;
    check(render({view(rgba,2,1),0},s,ctx).status==Status::Cancelled,"Cancellation ignored");ctx.cancelled=nullptr;
    std::vector<float> strided{0.1f,0.2f,0.3f,1,9,9,9,9, 0.6f,0.4f,0.2f,1};
    s.mix=0;r=render({view(strided,1,2,8),0},s,ctx);check(r.rgba==rgba,"Stride included padding");
    for(double x:{-0.2,0.0,0.0031308,0.18,1.0,2.0})near(srgb_to_linear(linear_to_srgb(x)),x,1e-12);
    s.mix=1;s.enhance=1;
    std::vector<std::future<std::vector<float>>> futures;
    for(int i=0;i<4;++i)futures.push_back(std::async(std::launch::async,[&,s](){RenderContext local;local.backend=backend;auto out=render({view(rgba,2,1),0},s,local);check(out.status==Status::Ok,"Parallel core render");return out.rgba;}));
    auto reference=futures[0].get();for(std::size_t i=1;i<futures.size();++i)check(futures[i].get()==reference,"Order/parallel mismatch");
    ctx.cached_source.reserve(4096);ctx.cached_restored.reserve(4096);ctx.cache_budget_bytes=64;
    r=render({view(rgba,2,1),0},s,ctx);
    check(r.status==Status::Ok&&!r.restoration_cache_hit,"Lowered physical cache budget retained hit");
    check((ctx.cached_source.capacity()+ctx.cached_restored.capacity())*sizeof(float)<=64,"Cache capacity exceeds budget");
    rejects([&](){std::istringstream large(std::string(10000,'x'));LUT::parse(large);});
    // Independent end-to-end color oracles, not calculated using core transfer helpers.
    std::vector<float> linear{0.18f,0.3f,0.6f,1};auto lv=view(linear,1,1);lv.transfer=Transfer::Linear;
    s.look.reset();s.look_amount=0;s.enhance=1;s.mix=1;
    r=render({lv,0},s,ctx);check(r.status==Status::Ok,"Linear model contract failed");
    for(std::size_t i=0;i<linear.size();++i)near(r.rgba[i],linear[i],2e-7);
    std::istringstream constant("LUT_3D_SIZE 2\n0.5 0.25 0.75\n0.5 0.25 0.75\n0.5 0.25 0.75\n0.5 0.25 0.75\n0.5 0.25 0.75\n0.5 0.25 0.75\n0.5 0.25 0.75\n0.5 0.25 0.75\n");
    s.look=std::make_shared<LUT>(LUT::parse(constant));s.enhance=0;s.look_amount=0.4;s.mix=0.5;
    std::vector<float> color{0.2f,0.4f,0.6f,1};r=render({view(color,1,1),0},s,ctx);
    check(r.status==Status::Ok,"Linear-light blend failed");
    near(r.rgba[0],0.2918996470110152);near(r.rgba[1],0.3756999255394099);near(r.rgba[2],0.6337273304888196);
    std::vector<float> premult{0.1f,0.2f,0.3f,0.5f};auto pm=view(premult,1,1);pm.alpha=Alpha::Premultiplied;
    s.look_amount=1;s.mix=1;r=render({pm,0},s,ctx);check(r.status==Status::Ok,"Premult nonidentity LUT failed");
    near(r.rgba[0],0.25);near(r.rgba[1],0.125);near(r.rgba[2],0.375);near(r.rgba[3],0.5);
    double max_decode=0,max_encode=0;
    for(int i=0;i<=200000;++i) {
        double x=double(i)/200000;
        max_decode=std::max(max_decode,std::abs(srgb_to_linear_fast(x)-srgb_to_linear(x)));
        max_encode=std::max(max_encode,std::abs(linear_to_srgb_fast(x)-linear_to_srgb(x)));
    }
    check(max_decode<=1e-8&&max_encode<=5e-7,"Fast transfer exceeded declared bound");
    for(double x:{-2.0,-.1,1.5,20.0}) {
        check(srgb_to_linear_fast(x)==srgb_to_linear(x),"Extended decode lost exact fallback");
        check(linear_to_srgb_fast(x)==linear_to_srgb(x),"Extended encode lost exact fallback");
    }
    s.fast_color_math=true;r=render({pm,0},s,ctx);
    check(r.status==Status::Ok,"Fast premult LUT failed");near(r.rgba[0],.25,1e-6);near(r.rgba[1],.125,1e-6);near(r.rgba[2],.375,1e-6);
}
void grade_tests(){
    GradeSettings s;
    RGB source{-0.0,-.3,8.0};auto out=grade_linear(source,s);
    check(std::signbit(out.r)&&out.g==source.g&&out.b==source.b,"Neutral grade lost exact samples");
    s.exposure_stops=2;out=grade_linear({-.25,.18,2},s);
    near(out.r,-1,1e-15);near(out.g,.72,1e-15);near(out.b,8,1e-15);
    s={};s.saturation=0;out=grade_linear({.2,.4,.6},s);
    near(out.r,.37192,1e-15);near(out.g,.37192,1e-15);near(out.b,.37192,1e-15);
    s.saturation=1.7;out=grade_linear({-.2,.4,1.6},s);
    near(.2126*out.r+.7152*out.g+.0722*out.b,.2126*(-.2)+.7152*.4+.0722*1.6,1e-15);
    s={};s.contrast=2;out=grade_linear({.37,.18,-.01},s);
    // Independently reduced integer-ratio oracle: 38^2/19=76; -(2^2*19)=-76.
    near(out.r,.75,1e-14);near(out.g,.18,1e-14);near(out.b,-.75,1e-14);
    out=grade_linear({0,1,2},s);near(out.r,-.18,1e-14);check(out.g>1&&out.b>out.g,"Contrast unexpectedly clamped extended values");
    s.exposure_stops=-1;out=grade_linear({.74,.74,.74},s);near(out.r,.75,1e-14);
    s={};s.contrast=.5;out=grade_linear({.75,0,-.01},s);
    near(out.r,.37,1e-14);near(out.g,.01*(std::sqrt(19.0)-1),1e-14);near(out.b,.01*(std::sqrt(9.5)-1),1e-14);
    s={1,2,.5};out=grade_linear({.185,.09,-.005},s);
    near(out.r,.492018,1e-14);near(out.g,.207018,1e-14);near(out.b,-.257982,1e-14);
    double last=-std::numeric_limits<double>::infinity();s={};s.contrast=1.3;
    for(int i=0;i<=1000;++i){double x=-.5+double(i)*.003;out=grade_linear({x,x,x},s);check(out.r>last,"Grade ramp not strictly monotonic");last=out.r;}
    for(auto invalid:{GradeSettings{11,1,1},GradeSettings{0,0,1},GradeSettings{0,1,3},GradeSettings{0,std::numeric_limits<double>::quiet_NaN(),1}})
        rejects([&]{grade_linear({.1,.2,.3},invalid);});
    rejects([&]{grade_linear({std::numeric_limits<double>::infinity(),0,0},GradeSettings{});});
    s={};s.contrast=3;rejects([&]{grade_linear({std::numeric_limits<double>::max(),0,0},s);});
    s={};s.contrast=.1;rejects([&]{grade_linear({1e307,0,0},s);});
    auto extreme=grade_linear({std::numeric_limits<float>::max(),0,0},s);
    check(std::isfinite(extreme.r)&&extreme.r>0,"Declared float-domain contrast overflowed");
    s={};s.exposure_stops=.75;s.contrast=1.2;s.saturation=.85;
    auto expected=grade_linear({-.05,.18,1.2},s);std::vector<std::future<RGB>> tasks;
    for(int i=0;i<8;++i)tasks.push_back(std::async(std::launch::async,[s]{return grade_linear({-.05,.18,1.2},s);}));
    for(auto& task:tasks){auto actual=task.get();check(actual.r==expected.r&&actual.g==expected.g&&actual.b==expected.b,"Pure grade changed under concurrency");}
    s={};s.temperature=1;out=grade_linear({.18,.18,.18},s);
    near(out.r/out.g,std::sqrt(2.0),1e-14);near(out.b/out.g,1/std::sqrt(2.0),1e-14);
    near(.2126*out.r+.7152*out.g+.0722*out.b,.18,1e-14);
    s={};s.tint=1;out=grade_linear({.18,.18,.18},s);
    near(out.r/out.g,std::sqrt(2.0),1e-14);near(out.b,out.r,1e-14);
    rejects([&]{grade_linear({.1,.2,.3},GradeSettings{0,1,1,2,0});});
}
}
int main(){try{lut_tests();shuffle_tests();pipeline_tests();grade_tests();std::cout<<"PASS "<<checks<<" bounded core assertions; AE/real-model validity not established\n";return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<" at assertion "<<checks<<"\n";return 1;}}
