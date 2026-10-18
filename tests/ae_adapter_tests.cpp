// Owned SDK-callback fixture. Exercises native adapter bytes; not the AE loader/runtime.
#include "plugin.hpp"
#include "AE_EffectUI.h"
#include <cstring>
#include <cmath>
#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <dlfcn.h>
#include <fstream>
#include "scene_cache_fixture.hpp"
#include <cstdlib>
#include <unistd.h>

namespace {
int checks=0;
PF_FilterProc effect_fn=nullptr;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
struct Fixture;
thread_local Fixture* active=nullptr;
struct Fixture {
    PF_InData in{};PF_OutData out{};SPBasicSuite basic{};PF_WorldSuite2 worlds{};
    std::array<PF_ParamDef,ParamCount> params{};
    PF_EffectWorld input{},output{};std::array<PF_EffectWorld*,5> temporal{};
    int checkout=0,checkin=0,acquire=0,release=0,pixels_out=0,pixels_in=0,abort_calls=0;
    std::vector<std::string> guids;bool fail_guid=false;
    PF_LRect maximum_rect{0,0,96,96};
    std::vector<int> layer_ids;std::vector<PF_LRect> requests;int fail_layer=-1,fail_pixel_id=-1;
    int fail_param=0;bool fail_suite=false,fail_output=false,fail_pixel_checkout=false,fail_format=false;
    bool fail_checkin=false,fail_pixel_checkin=false,fail_release=false,abort_now=false;
    Fixture();
};
Fixture& fixture(PF_ProgPtr ref){return *reinterpret_cast<Fixture*>(ref);}
PF_Err checkout_cb(PF_ProgPtr ref,PF_ParamIndex id,A_long,A_long,A_u_long,PF_ParamDef* value){auto& f=fixture(ref);++f.checkout;if(id==AdvancedStart||id==AdvancedEnd)return 74;if(id==f.fail_param)return 73;*value=f.params[id];return PF_Err_NONE;}
PF_Err checkin_cb(PF_ProgPtr ref,PF_ParamDef*){auto& f=fixture(ref);++f.checkin;return f.fail_checkin?74:PF_Err_NONE;}
PF_Err add(PF_ProgPtr ref,PF_ParamIndex,PF_ParamDef* value){auto& f=fixture(ref);f.params[++f.checkout]=*value;return PF_Err_NONE;}
PF_Err register_ui_cb(PF_ProgPtr,PF_CustomUIInfo* value){return value&&value->events==PF_CustomEFlag_EFFECT?PF_Err_NONE:PF_Err_BAD_CALLBACK_PARAM;}
PF_Err abort_cb(PF_ProgPtr ref){auto& f=fixture(ref);++f.abort_calls;return f.abort_now?75:PF_Err_NONE;}
SPErr acquire_cb(const char* name,int32 version,const void** suite){++active->acquire;if(active->fail_suite)return 76;if(std::strcmp(name,kPFWorldSuite)||version!=kPFWorldSuiteVersion2)return 77;*suite=&active->worlds;return 0;}
SPErr release_cb(const char*,int32){++active->release;return active->fail_release?78:0;}
PF_Err pixel_format(const PF_EffectWorld* world,PF_PixelFormat* format){if(active->fail_format)return 79;if(!world->width||!world->height)return 80;*format=static_cast<PF_PixelFormat>(reinterpret_cast<std::uintptr_t>(world->reserved0));return PF_Err_NONE;}
PF_Err checkout_pixels(PF_ProgPtr ref,A_long id,PF_EffectWorld** world){auto& f=fixture(ref);++f.pixels_out;if(f.fail_pixel_checkout||id==f.fail_pixel_id)return 81;*world=id>=0&&id<5&&f.temporal[id]?f.temporal[id]:&f.input;return PF_Err_NONE;}
PF_Err checkout_output(PF_ProgPtr ref,PF_EffectWorld** world){auto& f=fixture(ref);if(f.fail_output)return 82;*world=&f.output;return PF_Err_NONE;}
PF_Err checkin_pixels(PF_ProgPtr ref,A_long){auto& f=fixture(ref);++f.pixels_in;return f.fail_pixel_checkin?83:PF_Err_NONE;}
Fixture::Fixture(){
    in.appl_id=kAppID_AfterEffects;
    in.effect_ref=reinterpret_cast<PF_ProgPtr>(this);in.time_scale=24;in.time_step=1;in.pica_basicP=&basic;
    in.inter.checkout_param=checkout_cb;in.inter.checkin_param=checkin_cb;in.inter.add_param=add;in.inter.abort=abort_cb;in.inter.register_ui=register_ui_cb;
    basic.AcquireSuite=acquire_cb;basic.ReleaseSuite=release_cb;worlds.PF_GetPixelFormat=pixel_format;
    params[Source].u.pd.value=1;params[Look].u.pd.value=1;params[WorkingSpace].u.pd.value=3;
    params[Contrast].u.fs_d.value=1;params[Saturation].u.fs_d.value=100;params[LookAmount].u.fs_d.value=60;params[Mix].u.fs_d.value=100;
}
PF_Err run(Fixture& f,bool smart){
    active=&f;
    if(smart){PF_SmartRenderCallbacks cb{};cb.checkout_layer_pixels=checkout_pixels;cb.checkout_output=checkout_output;cb.checkin_layer_pixels=checkin_pixels;PF_SmartRenderExtra extra{};extra.cb=&cb;return effect_fn(PF_Cmd_SMART_RENDER,&f.in,&f.out,nullptr,nullptr,&extra);}
    f.params[Input].u.ld=f.input;std::array<PF_ParamDef*,ParamCount> list{};
    for(int i=0;i<ParamCount;++i)list[i]=&f.params[i];return effect_fn(PF_Cmd_RENDER,&f.in,&f.out,list.data(),&f.output,nullptr);
}
template<class P>void set_world(PF_EffectWorld& world,std::vector<P>& storage,A_long width,A_long height,A_long row,PF_PixelFormat format,A_long x=0,A_long y=0){
    world={};world.data=reinterpret_cast<PF_PixelPtr>(storage.data());world.width=width;world.height=height;world.rowbytes=row*static_cast<A_long>(sizeof(P));world.origin_x=x;world.origin_y=y;world.reserved0=reinterpret_cast<void*>(static_cast<std::uintptr_t>(format));
}
template<class P>void formats(PF_PixelFormat format,P pixel){
    Fixture f;std::vector<P> input{pixel,pixel,pixel,pixel,pixel,pixel,pixel,pixel},output(2);
    set_world(f.input,input,3,2,4,format,10,20);set_world(f.output,output,1,1,2,format,11,21);
    std::memset(output.data(),0x5b,output.size()*sizeof(P));P guard=output[1];
    check(run(f,true)==PF_Err_NONE,"Smart native bypass failed");
    check(std::memcmp(&output[0],&input[5],sizeof(P))==0,"Smart ROI/bypass sample mismatch");
    check(std::memcmp(&output[1],&guard,sizeof(P))==0,"Output row padding overwritten");
    check(f.checkout==RenderParamCount&&f.checkin==f.checkout&&f.acquire==1&&f.release==1&&f.pixels_in==1,"Successful native resources unbalanced");
    Fixture legacy;set_world(legacy.input,input,1,1,4,format);set_world(legacy.output,output,1,1,2,format);
    legacy.params[Mix].u.fs_d.value=0;legacy.params[Source].u.pd.value=2;legacy.params[WorkingSpace].u.pd.value=1;
    check(run(legacy,false)==PF_Err_NONE&&std::memcmp(&output[0],&input[0],sizeof(P))==0,"Legacy LogMix0 exact bypass failed");
}
void processing(){
    Fixture f;std::vector<PF_Pixel16> input{{16384,4096,8192,12288}},output(1);
    set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB64);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB64);
    f.params[Exposure].u.fs_d.value=1;
    check(run(f,true)==PF_Err_NONE,"Native16 grading failed");
    check(output[0].alpha==16384&&output[0].red==8192&&output[0].green==16384&&output[0].blue==24576,"SDK32768/premult scale wrong");
    Fixture unknown;unknown.params[Exposure].u.fs_d.value=1;unknown.params[WorkingSpace].u.pd.value=1;
    set_world(unknown.input,input,1,1,1,PF_PixelFormat_ARGB64);set_world(unknown.output,output,1,1,1,PF_PixelFormat_ARGB64);
    check(run(unknown,true)==PF_Err_BAD_CALLBACK_PARAM&&unknown.acquire==0&&unknown.checkin==RenderParamCount&&unknown.pixels_in==1,"Unknown space error/resources wrong");
}
void check_dynamic_flags(){
    // AE supplies its current flags. The query may only change documented dynamic bits;
    // this effect has no parameter-dependent capability changes, so preserve both words.
    const PF_OutFlags first[]{0,0x02008042,PF_OutFlag_WIDE_TIME_INPUT,PF_OutFlag_PIX_INDEPENDENT};
    const PF_OutFlags2 second[]{0,0x08221401,PF_OutFlag2_SUPPORTS_SMART_RENDER,PF_OutFlag2_REVEALS_ZERO_ALPHA};
    for(auto a:first)for(auto b:second){
        Fixture query;active=&query;query.out.out_flags=a;query.out.out_flags2=b;
        check(effect_fn(PF_Cmd_QUERY_DYNAMIC_FLAGS,&query.in,&query.out,nullptr,nullptr,nullptr)==0&&query.out.out_flags==a&&query.out.out_flags2==b,"Dynamic query changed host-provided flags");
    }
}
void cinema(){
    check_dynamic_flags();
    Fixture tone;tone.params[NaturalContrast].u.bd.value=TRUE;tone.params[Contrast].u.fs_d.value=2;tone.params[WorkingSpace].u.pd.value=3;
    std::vector<PF_PixelFloat> gray_in{{1,.5f,.5f,.5f}},gray_out(1);
    set_world(tone.input,gray_in,1,1,1,PF_PixelFormat_ARGB128);set_world(tone.output,gray_out,1,1,1,PF_PixelFormat_ARGB128);
    check(run(tone,true)==0&&std::abs(gray_out[0].red-.82f)<1e-6f,"Natural contrast failed native parameter/render path");
    Fixture ui;active=&ui;
    check(effect_fn(PF_Cmd_PARAMS_SETUP,&ui.in,&ui.out,nullptr,nullptr,nullptr)==PF_Err_NONE,"Cinema schema setup failed");
    check(ui.params[SmartColor].uu.id==13&&ui.params[SmartColor].u.bd.dephault==FALSE&&ui.params[CameraInfo].uu.id==12,"Smart default/legacy IDs wrong");
    check(ui.params[AdvancedStart].param_type==PF_Param_GROUP_START&&
          (ui.params[AdvancedStart].flags&PF_ParamFlag_COLLAPSE_TWIRLY)&&
          ui.params[AdvancedEnd].param_type==PF_Param_GROUP_END,"Advanced must start collapsed");
    check(ui.params[WorkingSpace].uu.id==9&&ui.params[WorkingSpace].u.pd.num_choices==4&&
          ui.params[WorkingSpace].u.pd.dephault==2&&ui.params[WorkingSpace].u.pd.value==2,"New effects must default to sRGB with legacy choices retained");
    // IEC sRGB: half of linear white encodes to0.735356983; linear0.5
    // would expose an incorrect new-effect transfer without reusing core math.
    std::vector<PF_PixelFloat> white{{1,1,1,1}},half_white(1);
    ui.params[Exposure].u.fs_d.value=-1;set_world(ui.input,white,1,1,1,PF_PixelFormat_ARGB128);set_world(ui.output,half_white,1,1,1,PF_PixelFormat_ARGB128);
    check(run(ui,true)==0&&std::abs(half_white[0].red-.735356983f)<1e-6f&&half_white[0].alpha==1,"Fresh sRGB effect must render without encoding selection");
    check(ui.params[Look].u.pd.num_choices==17&&ui.params[Look].uu.id==6,"Cinema menu or persistent ID wrong");
    check(ui.params[Source].uu.id==1&&ui.params[Source].u.pd.num_choices==8&&ui.params[Source].u.pd.dephault==1,"Input profile schema/legacy default changed");
    check(Source>AdvancedStart&&Source<AdvancedEnd,"Technical input profile must stay inside Advanced");
    check(ui.params[NaturalContrast].uu.id==16&&ui.params[NaturalContrast].u.bd.dephault==TRUE&&ui.params[NaturalContrast].u.bd.value==FALSE&&(ui.params[NaturalContrast].flags&PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS),"Natural contrast new/legacy default contract wrong");
    check(NaturalContrast>AdvancedStart&&NaturalContrast<AdvancedEnd,"Natural contrast must stay in Advanced");
    for(int profile=3;profile<=8;++profile){
        ui.params[Source].u.pd.value=profile;ui.params[Exposure].u.fs_d.value=0;ui.params[WorkingSpace].u.pd.value=3;
        auto code=float(fstr::linear_to_source_channel(.18,static_cast<fstr::SourceProfile>(profile-1)));white[0]={.5f,code*.5f,code*.5f,code*.5f};
        check(run(ui,true)==0&&std::abs(half_white[0].red-.09f)<2e-6f&&half_white[0].alpha==.5f,"Appended profile failed SDK callback/premult path");
    }
    check(std::strncmp(ui.params[Look].u.pd.u.namesptr,"Natural|(-|",11)==0,"Natural/retired Warm schema wrong");
    check(ui.params[LookAmount].u.fs_d.dephault==100&&ui.params[LookAmount].u.fs_d.slider_max==100&&ui.params[LookAmount].u.fs_d.valid_max==300,"Strength soft/hard/default range wrong");
    std::vector<PF_PixelFloat> input{{.5f,.15f,.22f,.3f}},output(1);
    for(int look=1;look<=17;++look){
        Fixture f;set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB128);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB128);
        f.params[Look].u.pd.value=look;f.params[LookAmount].u.fs_d.value=200;
        check(run(f,true)==PF_Err_NONE&&output[0].alpha==input[0].alpha,"Cinema choice/amplification failed");
        if(look==1)check(std::memcmp(input.data(),output.data(),sizeof(PF_PixelFloat))==0,"Natural native pixels changed");
        else check(std::memcmp(input.data(),output.data(),sizeof(PF_PixelFloat))!=0,"Selected LUT did not process native pixels");
    }
    for(int invalid:{0,18}){
        Fixture f;set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB128);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB128);f.params[Look].u.pd.value=invalid;
        check(run(f,true)==PF_Err_BAD_CALLBACK_PARAM,"Invalid look menu value accepted");
    }
}
PF_Err checkout_layer(PF_ProgPtr ref,PF_ParamIndex,A_long id,const PF_RenderRequest* request,A_long,A_long,A_u_long,PF_CheckoutResult* result){
 auto&f=fixture(ref);f.layer_ids.push_back(id);f.requests.push_back(request->rect);if(id==f.fail_layer)return 85;
 result->result_rect=request->rect;result->max_result_rect=f.maximum_rect;return PF_Err_NONE;
}
PF_Err mix_guid(PF_ProgPtr,A_u_long,const void*);
void smart_requests(){
 for(int mode=0;mode<6;++mode){
  Fixture f;active=&f;f.in.current_time=10;f.in.total_time=100;
  f.params[SmartColor].u.bd.value=mode!=0;f.params[Look].u.pd.value=3;
  PF_PreRenderInput pi{};pi.output_request.rect={10,10,20,20};PF_PreRenderOutput po{};PF_PreRenderCallbacks pc{};pc.checkout_layer=checkout_layer;pc.GuidMixInPtr=mix_guid;
  PF_PreRenderExtra pe{&pi,&po,&pc};if(mode==2)f.fail_layer=2;
  auto err=effect_fn(PF_Cmd_SMART_PRE_RENDER,&f.in,&f.out,nullptr,nullptr,&pe);
  if(mode==2){check(err==85&&!po.pre_render_data,"Pre-render failure leaked/succeeded");continue;}
  check(err==0,"Smart pre-render failed");
  check(f.layer_ids==(mode==0?std::vector<int>{0}:std::vector<int>{1000,0,1,2,3,4}),"Unexpected temporal/full-frame requests");
  check(po.result_rect.left==10&&po.result_rect.right==20,"Output ROI expanded");
  if(mode!=0)check(f.requests[1].left==0&&f.requests[1].right==96,"Analysis input remains cropped");
  std::vector<PF_Pixel> input(96*96,PF_Pixel{255,128,128,128}),output(1);
  set_world(f.input,input,96,96,96,PF_PixelFormat_ARGB32);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB32,10,10);
  // Zero Mix skips detection but exercises acquisition and every cleanup branch.
  f.params[Mix].u.fs_d.value=0;if(mode==3)f.fail_pixel_id=4;if(mode==4)f.fail_output=true;if(mode==5)f.abort_now=true;
  PF_SmartRenderInput si{};si.pre_render_data=po.pre_render_data;PF_SmartRenderCallbacks sc{};sc.checkout_layer_pixels=checkout_pixels;sc.checkin_layer_pixels=checkin_pixels;sc.checkout_output=checkout_output;PF_SmartRenderExtra se{};se.input=&si;se.cb=&sc;
  err=effect_fn(PF_Cmd_SMART_RENDER,&f.in,&f.out,nullptr,nullptr,&se);
  check((mode<2?err==0:err!=0),"Smart render failure propagation");
  check(f.pixels_in==f.pixels_out-(mode==3?1:0),"Temporal input cleanup imbalance");
  if(po.pre_render_data)po.delete_pre_render_data_func(po.pre_render_data);
 }
}
void automatic_scene(const char* config_path){
 std::ifstream file(config_path);const auto cfg=nlohmann::json::parse(file);const int w=cfg.at("width"),h=cfg.at("height"),source_height=cfg.at("source_height");const std::size_t count=std::size_t(w)*std::size_t(h);std::vector<unsigned char> bytes(count*4);std::ifstream raw(cfg.at("rgba8").get<std::string>(),std::ios::binary);raw.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()));check(bool(raw)&&raw.peek()==EOF,"Automatic frame input invalid");
 std::vector<PF_PixelFloat> input(count),off(count),on(count),again(count);for(std::size_t i=0;i<count;++i)input[i]={1,bytes[i*4]/255.f,bytes[i*4+1]/255.f,bytes[i*4+2]/255.f};Fixture f;f.in.pixel_aspect_ratio={1,1};f.in.downsample_x={h,static_cast<A_u_long>(source_height)};f.in.downsample_y={h,static_cast<A_u_long>(source_height)};f.params[WorkingSpace].u.pd.value=2;f.params[Look].u.pd.value=9;f.params[LookAmount].u.fs_d.value=100;set_world(f.input,input,w,h,w,PF_PixelFormat_ARGB128);set_world(f.output,off,w,h,w,PF_PixelFormat_ARGB128);check(run(f,true)==0,"Automatic Off render failed");

 std::array<std::vector<PF_PixelFloat>,5> adjacent;std::array<PF_EffectWorld,5> worlds;PF_PreRenderOutput po{};PF_SmartRenderInput si{};
 if(cfg.contains("neighbors")){
  check(cfg.at("neighbors").size()==4,"Automatic explicit window size");f.maximum_rect={0,0,w,h};f.in.time_scale=12;f.in.current_time=cfg.at("frame");f.in.total_time=cfg.at("total_frames");f.temporal[0]=&f.input;
  for(int id=1;id<5;++id){std::vector<unsigned char> data(count*4);std::ifstream source(cfg.at("neighbors")[id-1].get<std::string>(),std::ios::binary);source.read(reinterpret_cast<char*>(data.data()),std::streamsize(data.size()));check(bool(source)&&source.peek()==EOF,"Explicit neighbor input invalid");adjacent[id].resize(count);for(std::size_t i=0;i<count;++i)adjacent[id][i]={1,data[i*4]/255.f,data[i*4+1]/255.f,data[i*4+2]/255.f};set_world(worlds[id],adjacent[id],w,h,w,PF_PixelFormat_ARGB128);f.temporal[id]=&worlds[id];}
  f.params[SmartColor].u.bd.value=TRUE;active=&f;PF_PreRenderInput pi{};pi.output_request.rect={0,0,w,h};PF_PreRenderCallbacks pc{};pc.checkout_layer=checkout_layer;pc.GuidMixInPtr=mix_guid;PF_PreRenderExtra pe{&pi,&po,&pc};check(effect_fn(PF_Cmd_SMART_PRE_RENDER,&f.in,&f.out,nullptr,nullptr,&pe)==0,"Automatic explicit pre-render failed");si.pre_render_data=po.pre_render_data;
 }
 struct PreCleanup{PF_PreRenderOutput& value;~PreCleanup(){if(value.pre_render_data)value.delete_pre_render_data_func(value.pre_render_data);}} cleanup{po};
 auto render_on=[&]{if(!cfg.contains("neighbors"))return run(f,true);active=&f;PF_SmartRenderCallbacks cb{};cb.checkout_layer_pixels=checkout_pixels;cb.checkout_output=checkout_output;cb.checkin_layer_pixels=checkin_pixels;PF_SmartRenderExtra se{};se.input=&si;se.cb=&cb;return effect_fn(PF_Cmd_SMART_RENDER,&f.in,&f.out,nullptr,nullptr,&se);};
 f.params[SmartColor].u.bd.value=TRUE;set_world(f.output,on,w,h,w,PF_PixelFormat_ARGB128);check(render_on()==0,"Automatic On render failed");std::size_t changed=0;for(std::size_t i=0;i<count;++i){if(std::memcmp(&off[i],&on[i],sizeof(PF_PixelFloat)))++changed;check(on[i].alpha==1,"Automatic alpha changed");}check(changed>0,"Automatic Smart inactive");
 set_world(f.output,again,w,h,w,PF_PixelFormat_ARGB128);f.params[SmartColor].u.bd.value=FALSE;check(run(f,true)==0&&std::memcmp(off.data(),again.data(),count*sizeof(PF_PixelFloat))==0,"Automatic Off repeat differs");f.params[SmartColor].u.bd.value=TRUE;check(render_on()==0&&std::memcmp(on.data(),again.data(),count*sizeof(PF_PixelFloat))==0,"Automatic On repeat differs");

 if(!cfg.value("reference_only",false)){
 f.params[Look].u.pd.value=10;check(run(f,true)==0,"Automatic changed look failed");f.params[LookAmount].u.fs_d.value=150;check(run(f,true)==0,"Automatic changed amount failed");f.params[Look].u.pd.value=9;f.params[LookAmount].u.fs_d.value=100;
 input[0].red+=1.f/255;check(run(f,true)==0,"Automatic changed upstream failed");input[0].red-=1.f/255;input[0].red+=1e-7f;check(run(f,true)==0,"Sub-8-bit upstream change failed");input[0].red-=1e-7f;
 }
 auto save=[&](const char*name,const auto&v){std::ofstream out(cfg.at("output").get<std::string>()+name,std::ios::binary);for(auto p:v){float rgba[]{p.red,p.green,p.blue,p.alpha};out.write(reinterpret_cast<char*>(rgba),sizeof(rgba));}check(bool(out),"Automatic fixture output failed");};save("-on.rgba32f",on);save("-off.rgba32f",off);
 std::cout<<"Automatic native Smart changes "<<changed<<" pixels; switches exact; look/amount/upstream render PASS\n";
}
void real_smart(const char* path){
 std::ifstream stream(path,std::ios::binary);std::vector<unsigned char> bytes(540*960*4);stream.read(reinterpret_cast<char*>(bytes.data()),bytes.size());check(bool(stream),"Real fixture missing");
 std::vector<PF_Pixel> input(540*960),off(input.size()),on(input.size());
 for(std::size_t i=0;i<input.size();++i)input[i]={255,bytes[4*i],bytes[4*i+1],bytes[4*i+2]};
 Fixture f;set_world(f.input,input,540,960,540,PF_PixelFormat_ARGB32);set_world(f.output,off,540,960,540,PF_PixelFormat_ARGB32);
 f.in.downsample_y={1,4};f.in.downsample_x={1,4};f.in.pixel_aspect_ratio={1,1};f.params[WorkingSpace].u.pd.value=2;f.params[Look].u.pd.value=3;f.params[LookAmount].u.fs_d.value=100;
 check(run(f,true)==0,"Real Smart Off failed");f.params[SmartColor].u.bd.value=TRUE;set_world(f.output,on,540,960,540,PF_PixelFormat_ARGB32);
 check(run(f,true)==0,"Real Smart On failed");std::size_t changed=0;
 for(std::size_t i=0;i<input.size();++i){check(on[i].alpha==255,"Smart changed alpha");if(std::memcmp(&off[i],&on[i],sizeof(PF_Pixel)))++changed;}
 check(changed>0&&changed<input.size()/10,"Smart protection absent or paints whole image");
 f.params[SmartColor].u.bd.value=FALSE;check(run(f,true)==0&&std::memcmp(off.data(),on.data(),off.size()*sizeof(PF_Pixel))==0,"Off does not restore original LUT output");
 std::cout<<"Real native Smart changes "<<changed<<" pixels; Off repeat exact\n";
}
void real_recovery(const char* path,int center=50,bool expected_recovery=true){
 Fixture f;active=&f;constexpr int width=540,height=960;const std::size_t count=width*height;
 std::ifstream file(path,std::ios::binary);std::array<std::vector<PF_Pixel>,5> pixels;std::array<PF_EffectWorld,5> worlds;
 std::vector<unsigned char> bytes(count*4);int offsets[]{0,-1,1,-2,2};
 for(int id=0;id<5;++id){file.seekg(std::size_t(center+offsets[id])*bytes.size());file.read(reinterpret_cast<char*>(bytes.data()),bytes.size());check(bool(file),"Sequence fixture read");pixels[id].resize(count);for(std::size_t j=0;j<count;++j)pixels[id][j]={255,bytes[j*4],bytes[j*4+1],bytes[j*4+2]};set_world(worlds[id],pixels[id],width,height,width,PF_PixelFormat_ARGB32);f.temporal[id]=&worlds[id];}
 f.input=worlds[0];std::vector<PF_Pixel> raw(count),recovered(count);set_world(f.output,raw,width,height,width,PF_PixelFormat_ARGB32);
 f.params[WorkingSpace].u.pd.value=2;f.params[Look].u.pd.value=3;f.params[LookAmount].u.fs_d.value=100;f.params[SmartColor].u.bd.value=TRUE;f.in.downsample_y={1,4};f.in.downsample_x={1,4};f.in.pixel_aspect_ratio={1,1};f.in.current_time=center;f.in.total_time=156;
 if(!expected_recovery)f.params[SmartColor].u.bd.value=FALSE;
 check(run(f,true)==0,"Current-only real frame failed");
 f.params[SmartColor].u.bd.value=TRUE;
 PF_PreRenderInput pi{};pi.output_request.rect={0,0,width,height};PF_PreRenderOutput po{};PF_PreRenderCallbacks pc{};pc.checkout_layer=checkout_layer;pc.GuidMixInPtr=mix_guid;PF_PreRenderExtra pe{&pi,&po,&pc};check(effect_fn(PF_Cmd_SMART_PRE_RENDER,&f.in,&f.out,nullptr,nullptr,&pe)==0,"Recovery pre-render failed");
 PF_SmartRenderInput si{};si.pre_render_data=po.pre_render_data;PF_SmartRenderCallbacks sc{};sc.checkout_layer_pixels=checkout_pixels;sc.checkin_layer_pixels=checkin_pixels;sc.checkout_output=checkout_output;PF_SmartRenderExtra se{};se.input=&si;se.cb=&sc;set_world(f.output,recovered,width,height,width,PF_PixelFormat_ARGB32);
 auto err=effect_fn(PF_Cmd_SMART_RENDER,&f.in,&f.out,nullptr,nullptr,&se);po.delete_pre_render_data_func(po.pre_render_data);check(err==0,"Actual temporal recovery failed");
 std::size_t changed=0;for(std::size_t i=0;i<count;++i){check(recovered[i].alpha==255,"Recovered alpha changed");if(std::memcmp(&raw[i],&recovered[i],sizeof(PF_Pixel)))++changed;}
 check(expected_recovery?(changed>0&&changed<count/10):changed==0,"Short recovery / long-gap plain LUT contract failed");check(f.pixels_in==f.pixels_out,"Real temporal checkout leak");std::cout<<"Real frame"<<center<<(expected_recovery?" temporal recovery changed ":" long-gap vs Smart Off changed ")<<changed<<" pixels\n";
}
void failures(){
    for(int mode=0;mode<12;++mode){
        Fixture f;std::vector<PF_Pixel> input{{255,10,20,30}},output(1);
        set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB32);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB32);
        if(mode==0)f.fail_param=4;
        if(mode==10)f.fail_param=WorkingSpace;
        if(mode==11)f.fail_param=SmartColor;
        if(mode==1)f.fail_suite=true;
        if(mode==2)f.fail_output=true;
        if(mode==3)f.fail_pixel_checkout=true;
        if(mode==4)f.fail_format=true;
        if(mode==5)f.fail_checkin=true;
        if(mode==6)f.fail_pixel_checkin=true;
        if(mode==7)f.fail_release=true;
        if(mode==8)f.abort_now=true;
        if(mode==9){f.params[Source].u.pd.value=2;f.params[Look].u.pd.value=2;f.params[Exposure].u.fs_d.value=11;}
        check(run(f,true)!=PF_Err_NONE,"Native injected failure silently succeeded");
        check(f.checkin==(f.fail_param?f.fail_param-1-(f.fail_param>AdvancedStart?1:0)-(f.fail_param>AdvancedEnd?1:0):(mode==2||mode==3?0:RenderParamCount)),"Parameter failure cleanup wrong");
        check(f.pixels_in==(f.fail_pixel_checkout?0:1),"Pixel failure cleanup wrong");
        check(f.release==(f.acquire&&!f.fail_suite?1:0),"Suite failure cleanup wrong");
    }
    Fixture empty;std::vector<PF_PixelFloat> input,output(1);set_world(empty.input,input,0,0,0,PF_PixelFormat_ARGB128);set_world(empty.output,output,1,1,1,PF_PixelFormat_ARGB128);
    check(run(empty,true)==PF_Err_NONE&&output[0].alpha==0,"Empty input must be transparent without querying null pixel format");
    empty.output.width=empty.output.height=0;check(run(empty,true)==PF_Err_NONE,"Empty output failed");
}
PF_Err mix_guid(PF_ProgPtr ref,A_u_long n,const void* data){auto&f=fixture(ref);f.guids.emplace_back(static_cast<const char*>(data),n);return f.fail_guid?91:0;}
void cached_scene_bridge(){
    char pattern[]="/tmp/fstr-native-scene-XXXXXX";auto name=mkdtemp(pattern);check(name!=nullptr,"Scene fixture directory");const std::filesystem::path root(name);
    struct Guard{std::filesystem::path path;~Guard(){unsetenv("FSTR_LUT_SCENE_CACHE");unsetenv("FSTR_LUT_SCENE_POLICY");std::error_code e;std::filesystem::remove_all(path,e);}} guard{root};
    const std::string policy(64,'a');fstr::AnalysisImage image{2,1,{120,70,40,255,180,120,90,255}};const auto key=scene_fixture::write(root,image,1,policy,true);
    setenv("FSTR_LUT_SCENE_CACHE",root.c_str(),1);setenv("FSTR_LUT_SCENE_POLICY",policy.c_str(),1);
    Fixture setup;active=&setup;check(effect_fn(PF_Cmd_GLOBAL_SETUP,&setup.in,&setup.out,nullptr,nullptr,nullptr)==0,"Scene global setup");
    {Fixture f;active=&f;f.in.current_time=5;f.in.total_time=10;f.params[SmartColor].u.bd.value=TRUE;f.params[Look].u.pd.value=9;
     PF_PreRenderInput input{};input.output_request.rect={0,0,2,1};PF_PreRenderOutput output{};PF_PreRenderCallbacks cb{};cb.checkout_layer=checkout_layer;cb.GuidMixInPtr=mix_guid;PF_PreRenderExtra extra{&input,&output,&cb};
     check(effect_fn(PF_Cmd_SMART_PRE_RENDER,&f.in,&f.out,nullptr,nullptr,&extra)==0,"Cached pre-render failed");check(f.guids.size()==1&&f.guids[0].rfind("FSTR_NATIVE_BASE_V2/",0)==0&&f.guids[0].size()>80&&f.layer_ids==std::vector<int>({1000,Input}),"Scene snapshot GUID or current-only checkout missing");if(output.delete_pre_render_data_func)output.delete_pre_render_data_func(output.pre_render_data);
     f.fail_guid=true;f.layer_ids.clear();check(effect_fn(PF_Cmd_SMART_PRE_RENDER,&f.in,&f.out,nullptr,nullptr,&extra)==91&&f.layer_ids.empty(),"GUID failure did not propagate before checkout");}
    auto check_look=[&](int look,const char* file){
        Fixture f;std::vector<PF_PixelFloat> input{{1,120.f/255,70.f/255,40.f/255},{1,180.f/255,120.f/255,90.f/255}},output(2);
        set_world(f.input,input,2,1,2,PF_PixelFormat_ARGB128);set_world(f.output,output,2,1,2,PF_PixelFormat_ARGB128);
        f.params[WorkingSpace].u.pd.value=2;f.params[Look].u.pd.value=look;f.params[LookAmount].u.fs_d.value=100;f.params[SmartColor].u.bd.value=TRUE;
        std::ifstream cube(std::string(FSTR_TEST_LOOK_ROOT)+"/"+file);auto lut=std::make_shared<fstr::LUT>(fstr::LUT::parse(cube));fstr::SceneLook scene(lut,1);auto reference=scene.skin_reference(std::vector<fstr::SkinSample>(40,{{.23f,.12f,.07f},1}));fstr::SceneWeights roles{};roles[0]=1;
        check(run(f,true)==0,"Cached native render failed");
        std::array<std::array<float,4>,2> expected;
        for(std::size_t i=0;i<2;++i){auto v=scene.apply_stable_people({float(fstr::srgb_to_linear(input[i].red)),float(fstr::srgb_to_linear(input[i].green)),float(fstr::srgb_to_linear(input[i].blue)),1},roles,{{&reference,1}},{{.75,.5}});for(std::size_t c=0;c<3;++c)v[c]=float(fstr::linear_to_srgb(v[c]));expected[i]=v;check(std::abs(output[i].red-v[0])<2e-6&&std::abs(output[i].green-v[1])<2e-6&&std::abs(output[i].blue-v[2])<2e-6,"AE bridge differs from independent scene core");}
        auto full=output;
        f.params[SmartColor].u.bd.value=FALSE;check(run(f,true)==0,"Scene Off failed");check(std::memcmp(output.data(),full.data(),output.size()*sizeof(PF_PixelFloat))!=0,"Native scene has no effect");auto ordinary=output;
        f.params[SmartColor].u.bd.value=TRUE;check(run(f,true)==0&&std::memcmp(output.data(),full.data(),output.size()*sizeof(PF_PixelFloat))==0,"On/Off/On cache changed output");
        for(auto&p:input){p.alpha*=.5f;p.red*=.5f;p.green*=.5f;p.blue*=.5f;}check(run(f,true)==0,"Premultiplied scene failed");
        for(std::size_t i=0;i<2;++i)check(output[i].alpha==.5f&&std::abs(output[i].red-full[i].red*.5f)<2e-6&&std::abs(output[i].green-full[i].green*.5f)<2e-6&&std::abs(output[i].blue-full[i].blue*.5f)<2e-6,"Cached scene broke premultiplied alpha");
        for(auto&p:input){p.alpha*=2;p.red*=2;p.green*=2;p.blue*=2;}
        std::vector<PF_PixelFloat> roi(2);roi[1]={.7f,.8f,.9f,.6f};set_world(f.output,roi,1,1,2,PF_PixelFormat_ARGB128,1,0);check(run(f,true)==0&&std::abs(roi[0].red-full[1].red)<2e-6&&roi[1].alpha==.7f,"Cached scene ROI/row padding changed");
        set_world(f.output,output,2,1,2,PF_PixelFormat_ARGB128);input[0].red+=1.f/255;check(run(f,true)==0,"Stale cache render failed");auto stale=output;f.params[SmartColor].u.bd.value=FALSE;check(run(f,true)==0&&std::memcmp(output.data(),stale.data(),output.size()*sizeof(PF_PixelFloat))==0,"Changed upstream reused stale scene mask");
    };
    check_look(9,"teal-orange-v1.cube");
    std::filesystem::rename(root/(key+".fstrscene"),root/(key+".hidden"));
    check_look(10,"neon-pink-cyan-v1.cube"); // Different LUT reuses immutable RAM analysis even without a ready header.
    check(effect_fn(PF_Cmd_GLOBAL_SETDOWN,&setup.in,&setup.out,nullptr,nullptr,nullptr)==0,"Scene setdown failed");
}

}
void* cleanup_module=nullptr;
int main(int argc,char** argv){try{
    check(argc>=2&&argc<=4,"Provide the actual built plugin executable");
    auto module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);cleanup_module=module;check(module!=nullptr,"Native plugin dlopen failed");
    effect_fn=reinterpret_cast<PF_FilterProc>(dlsym(module,"EffectMain"));check(effect_fn!=nullptr,"EffectMain export missing");
    Fixture setup;active=&setup;check(effect_fn(PF_Cmd_GLOBAL_SETUP,&setup.in,&setup.out,nullptr,nullptr,nullptr)==PF_Err_NONE,"Global setup failed");
    check(setup.out.out_flags==0x02008042&&setup.out.out_flags2==0x08221401&&setup.out.my_version==32769,"PiPL/runtime flags mismatch");
    check_dynamic_flags();
    Fixture ui;active=&ui;check(effect_fn(PF_Cmd_PARAMS_SETUP,&ui.in,&ui.out,nullptr,nullptr,nullptr)==PF_Err_NONE&&ui.out.num_params==ParamCount,"Native control schema failed");
    check(ui.params[Contrast].uu.id==3&&ui.params[WorkingSpace].uu.id==9&&ui.params[Temperature].uu.id==10&&ui.params[Tint].uu.id==11,"Persistent IDs changed with UI order");
    Fixture non_ae;non_ae.in.appl_id=0;check(effect_fn(PF_Cmd_GLOBAL_SETUP,&non_ae.in,&non_ae.out,nullptr,nullptr,nullptr)==PF_Err_INVALID_CALLBACK,"Non-AE host accepted");
    formats(PF_PixelFormat_ARGB32,PF_Pixel{127,51,75,99});
    formats(PF_PixelFormat_ARGB64,PF_Pixel16{12345,65535,7777,5123});
    formats(PF_PixelFormat_ARGB128,PF_PixelFloat{.5f,-.2f,2.0f,.1f});processing();cinema();failures();smart_requests();if(argc==4&&std::string(argv[2])=="--automatic"){std::ifstream jobs(argv[3]);auto batch=nlohmann::json::parse(jobs);if(batch.contains("jobs")){for(const auto&path:batch.at("jobs"))automatic_scene(path.get<std::string>().c_str());}else automatic_scene(argv[3]);}else {if(argc>=3)real_smart(argv[2]);if(argc==4){real_recovery(argv[3]);for(int frame:{54,55,56})real_recovery(argv[3],frame,false);}}
    std::vector<std::future<bool>> tasks;
    for(int i=0;i<8;++i)tasks.push_back(std::async(std::launch::async,[]{Fixture f;std::vector<PF_PixelFloat> input{{1,.1f,.2f,.3f}},output(1);set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB128);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB128);f.params[Exposure].u.fs_d.value=1;return run(f,true)==PF_Err_NONE&&output[0].red==.2f&&f.checkin==RenderParamCount&&f.pixels_in==1;}));
    for(auto& t:tasks)check(t.get(),"Parallel native snapshot mismatch");
    check(effect_fn(PF_Cmd_GLOBAL_SETDOWN,&setup.in,&setup.out,nullptr,nullptr,nullptr)==PF_Err_NONE,"Global setdown failed");
    cached_scene_bridge();
    check(dlclose(module)==0,"Native module cleanup failed");cleanup_module=nullptr;
    std::cout<<"PASS "<<checks<<" owned native callback assertions; actual AE loader/MFR: NOT RUN\n";return 0;
}catch(const std::exception& e){if(cleanup_module&&effect_fn){Fixture cleanup;active=&cleanup;effect_fn(PF_Cmd_GLOBAL_SETDOWN,&cleanup.in,&cleanup.out,nullptr,nullptr,nullptr);dlclose(cleanup_module);cleanup_module=nullptr;}std::cerr<<"FAIL "<<e.what()<<" assertion "<<checks<<"\n";return 1;}}
