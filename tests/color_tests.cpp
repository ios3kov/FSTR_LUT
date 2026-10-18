#include "fstr/color.hpp"
#include <cmath>
#include <cstring>
#include <cstdint>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace fstr;
namespace {
int checks=0;
void check(bool value,const char* text){++checks;if(!value)throw std::runtime_error(text);}
void near(double a,double b,double e=2e-7){check(std::abs(a-b)<=e,"Color numeric oracle mismatch");}
FrameView view(const std::vector<float>& v,std::size_t w=1,std::size_t h=1,std::size_t stride=4){return {v.data(),v.size(),w,h,stride};}
RenderResult ok(RenderResult r){check(r.status==Status::Ok&&r.rgba.size()>=4,"Expected successful complete color frame");return r;}
void profiles() {
    // New popup slot3: explicitly selected ARRI LogC3 EI800, not Host/Sony.
    ColorSettings arri;arri.source=static_cast<SourceProfile>(2);
    ColorProcessor decoder(arri,Transfer::Linear,Primaries::SRGB_Rec709,Alpha::Straight);
    const auto black=decoder.apply({.092809f,.092809f,.092809f,1});
    near(black[0],0,1e-7);near(black[1],0,1e-7);near(black[2],0,1e-7);
    near(source_channel_to_linear(.125,SourceProfile::PanasonicVLog),0,1e-15);
    near(source_channel_to_linear(.181,SourceProfile::PanasonicVLog),.01,1e-6);
    near(source_channel_to_linear(95./1023,SourceProfile::ArriLogC4),0,1e-14);
    near(source_channel_to_linear(.0729055341958355,SourceProfile::ACEScct),0,1e-15);
    near(source_channel_to_linear(.155251141552511,SourceProfile::ACEScct),.0078125,1e-14);
    near(source_channel_to_linear(4,SourceProfile::ACEScct),65504,1e-10);
    for(auto p:{SourceProfile::ArriLogC3EI800,SourceProfile::ArriLogC4,SourceProfile::PanasonicVLog,SourceProfile::ACEScct,SourceProfile::ACEScg}){
        for(double x:{-.05,0.,.0001,.01,.18,1.,16.})near(source_channel_to_linear(linear_to_source_channel(x,p),p),x,2e-6);
        auto gray=source_to_linear_rec709({linear_to_source_channel(.18,p),linear_to_source_channel(.18,p),linear_to_source_channel(.18,p)},p);
        near(gray.r,.18,1e-6);near(gray.g,.18,1e-6);near(gray.b,.18,1e-6);
        ColorSettings settings;settings.source=p;settings.mix=0;
        const std::array<float,4> raw{{-.25f,.5f,2.f,.25f}};
        check(ColorProcessor(settings,Transfer::SRGB,Primaries::SRGB_Rec709,Alpha::Premultiplied).apply(raw)==raw,"New profile Mix0 changed pixels");
        settings.mix=1;ColorProcessor process(settings,Transfer::Linear,Primaries::SRGB_Rec709,Alpha::Premultiplied);
        auto code=float(linear_to_source_channel(.18,p));auto v=process.apply({code*.25f,code*.25f,code*.25f,.25f});
        near(v[0],.045,2e-7);near(v[1],.045,2e-7);near(v[2],.045,2e-7);near(v[3],.25,1e-15);
        check(process.apply({4,3,2,0})==std::array<float,4>{{4,3,2,0}},"New profile changed hidden RGB");
        bool rejected=false;try{source_channel_to_linear(std::numeric_limits<double>::infinity(),p);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Nonfinite source accepted");
    }
    // Independent Sony published black/middle gray/white codes, not just reciprocal tests.
    near(slog3_to_linear(95.0/1023),0,1e-15);
    near(slog3_to_linear(420.0/1023),.18,1e-14);
    near(linear_to_slog3(0)*1023,95,1e-12);
    near(linear_to_slog3(.18)*1023,420,1e-12);
    near(linear_to_slog3(.9)*1023,598,.5); // Published integer code, rounded.
    near(slog3_to_linear(171.2102946929/1023),.01125,1e-13);
    auto gray=sony_cine_to_rec709({.18,.18,.18});near(gray.r,.18,1e-14);near(gray.g,.18,1e-14);near(gray.b,.18,1e-14);
    auto red=sony_cine_to_rec709({.18,0,0});
    near(red.r,.2928505337512344,1e-14);near(red.g,-.0321327948806778,1e-14);near(red.b,-.00799850070167355,1e-14);
    for(double x:{-.05,0.0,.00001,.01125,.18,.9,1.0,16.0,256.0})near(slog3_to_linear(linear_to_slog3(x)),x,1e-10);
    for(Transfer t:{Transfer::SRGB,Transfer::Linear,Transfer::Gamma24})
        for(double x:{-.3,0.0,.003,.18,1.0,4.0})near(decode_host(encode_host(x,t),t),x,1e-12);
    near(decode_host(.5,Transfer::Gamma24),.18946457081379978,1e-14);
    double previous=-1e9;
    for(int i=0;i<=2000;++i){double output=slog3_to_linear(-.1+i*.0006);check(output>previous,"SLog3 curve not monotonic");previous=output;}
}
void frames() {
    ColorContext c;ColorSettings s;
    std::vector<float> input{-0.0f,-.25f,2.0f,.5f};auto f=view(input);f.alpha=Alpha::Premultiplied;
    auto r=ok(render_color({f,0},s,c));check(r.status==Status::Ok&&std::memcmp(input.data(),r.rgba.data(),16)==0,"Neutral color lost exact samples");
    s.source=SourceProfile::SonySLog3Cine;s.mix=0;
    r=ok(render_color({f,0},s,c));check(r.status==Status::Ok&&std::memcmp(input.data(),r.rgba.data(),16)==0,"Log Mix0 not exact bypass");
    const std::uint32_t bits[]{0x80000000u,0x7fc12345u,0x7f800000u,0x3f800000u,0,0,0,0,0x7fa12345u,0xff800000u,0x40000000u,0};
    std::vector<float> special(12);std::memcpy(special.data(),bits,sizeof(bits));
    f=view(special,1,2,8);r=ok(render_color({f,0},s,c));
    check(r.rgba.size()==8&&std::memcmp(r.rgba.data(),special.data(),16)==0&&std::memcmp(r.rgba.data()+4,special.data()+8,16)==0,"Mix0 changed raw float object bytes or copied padding");
    s={};s.grade.exposure_stops=1;
    input={.09f,.18f,.36f,1};f=view(input);f.transfer=Transfer::Linear;
    r=ok(render_color({f,0},s,c));check(r.status==Status::Ok,"Linear grade failed");near(r.rgba[0],.18);near(r.rgba[1],.36);near(r.rgba[2],.72);near(r.rgba[3],1);
    s.mix=.25;r=ok(render_color({f,0},s,c));near(r.rgba[0],.1125);near(r.rgba[1],.225);near(r.rgba[2],.45);
    input={.045f,.09f,.18f,.5f};f=view(input);f.transfer=Transfer::Linear;f.alpha=Alpha::Premultiplied;s.mix=1;
    r=ok(render_color({f,0},s,c));check(r.status==Status::Ok,"Premult grade failed");near(r.rgba[0],.09);near(r.rgba[1],.18);near(r.rgba[2],.36);near(r.rgba[3],.5);
    input={95.0f/1023,420.0f/1023,420.0f/1023,1};f=view(input);f.transfer=Transfer::Linear;s={};s.source=SourceProfile::SonySLog3Cine;
    r=ok(render_color({f,0},s,c));check(r.status==Status::Ok,"True Sony color normalization failed");
    // Red differs from other channels: catches transposed/wrong gamut matrix and no gamut conversion.
    near(r.rgba[0],-.1128505337512344);near(r.rgba[1],.2121327948806778);near(r.rgba[2],.18799850070167355);
    input={420.0f/1023,420.0f/1023,420.0f/1023,1};f=view(input);f.transfer=Transfer::Linear;
    r=ok(render_color({f,0},s,c));near(r.rgba[0],.18);s.mix=.5;r=ok(render_color({f,0},s,c));near(r.rgba[0],.5*(420.0/1023+.18));
    f.transfer=Transfer::SRGB;s.mix=1;r=ok(render_color({f,0},s,c));near(r.rgba[0],.4613561295004416);
    std::vector<float> hidden{-2,4,.75f,0};f=view(hidden);s.grade.exposure_stops=3;
    r=ok(render_color({f,0},s,c));check(r.status==Status::Ok&&r.rgba==hidden,"Transparent hidden Log RGB changed");
    s={};s.grade.exposure_stops=1;f=view(input);f.primaries=Primaries::Unknown;
    check(render_color({f,0},s,c).status==Status::Unsupported,"Unknown host gamut silently accepted");
    f=view(input);f.count=3;check(render_color({f,0},s,c).status==Status::InvalidInput,"Short color buffer accepted");
    std::vector<float> stride_input(8);f=view(stride_input,1,2,std::numeric_limits<std::size_t>::max());
    check(render_color({f,0},s,c).status==Status::InvalidInput,"Color stride overflow accepted");
    f=view(input);check(render_color({f,1},s,c).status==Status::Unsupported,"Temporal color window silently ignored");
    f.memory=Memory::GPU;check(render_color({f,0},s,c).status==Status::Unsupported,"GPU color memory treated as CPU");
    std::atomic_bool cancelled{true};c.cancelled=&cancelled;f=view(input);
    check(render_color({f,0},s,c).status==Status::Cancelled,"Color cancellation ignored");c.cancelled=nullptr;
    s.source=SourceProfile::SonySLog3Cine;input[0]=10000;f=view(input);
    check(render_color({f,0},s,c).status==Status::InvalidInput,"Nonfinite Log decode accepted");
    s={};input={-.2f,2.0f,4.0f,1};f=view(input);f.transfer=Transfer::Linear;s.grade.exposure_stops=1;
    r=ok(render_color({f,0},s,c));near(r.rgba[0],-.4);near(r.rgba[1],4);near(r.rgba[2],8);
    s={};s.shoulder=1;input={2,2,2,1};f=view(input);f.transfer=Transfer::Linear;
    r=ok(render_color({f,0},s,c));near(r.rgba[0],.9111111111111111);near(r.rgba[1],r.rgba[0]);
    input={1e20f,1e20f,1e20f,1};f=view(input);f.transfer=Transfer::Linear;
    r=ok(render_color({f,0},s,c));check(r.status==Status::Ok,"Large finite shoulder failed");near(r.rgba[0],1,1e-6);
    input={.3f,.3f,.3f,1};f=view(input);f.transfer=Transfer::Linear;
    r=ok(render_color({f,0},s,c));check(r.rgba==input,"Shoulder changed values below knee");
    s={};s.grade.exposure_stops=.5;s.grade.saturation=.8;s.look=std::make_shared<LUT>(LUT::identity(17));
    std::vector<float> strided{.2f,.3f,.4f,1,99,99,99,99,.3f,.4f,.5f,1};f=view(strided,1,2,8);
    const auto expected=render_color({f,0},s,c);check(expected.status==Status::Ok&&expected.rgba.size()==8,"Strided color frame failed");
    std::vector<std::future<RenderResult>> tasks;
    for(int i=0;i<8;++i)tasks.push_back(std::async(std::launch::async,[&]{return render_color({f,0},s,c);}));
    for(auto& t:tasks){auto actual=t.get();check(actual.status==Status::Ok&&actual.rgba==expected.rgba,"Color render depends on order/thread");}
}
}
int main(){try{profiles();frames();std::cout<<"PASS "<<checks<<" color-only assertions; AE/profile loader: NOT RUN\n";return 0;}
catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<" assertion "<<checks<<"\n";return 1;}}
