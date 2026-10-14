// Owned SDK-callback fixture. Exercises native adapter bytes; not the AE loader/runtime.
#include "plugin.hpp"
#include <cstring>
#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <dlfcn.h>

namespace {
int checks=0;
PF_FilterProc effect_fn=nullptr;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
struct Fixture;
thread_local Fixture* active=nullptr;
struct Fixture {
    PF_InData in{};PF_OutData out{};SPBasicSuite basic{};PF_WorldSuite2 worlds{};
    std::array<PF_ParamDef,ParamCount> params{};
    PF_EffectWorld input{},output{};
    int checkout=0,checkin=0,acquire=0,release=0,pixels_out=0,pixels_in=0,abort_calls=0;
    int fail_param=0;bool fail_suite=false,fail_output=false,fail_pixel_checkout=false,fail_format=false;
    bool fail_checkin=false,fail_pixel_checkin=false,fail_release=false,abort_now=false;
    Fixture();
};
Fixture& fixture(PF_ProgPtr ref){return *reinterpret_cast<Fixture*>(ref);}
PF_Err checkout_cb(PF_ProgPtr ref,PF_ParamIndex id,A_long,A_long,A_u_long,PF_ParamDef* value){auto& f=fixture(ref);++f.checkout;if(id==f.fail_param)return 73;*value=f.params[id];return PF_Err_NONE;}
PF_Err checkin_cb(PF_ProgPtr ref,PF_ParamDef*){auto& f=fixture(ref);++f.checkin;return f.fail_checkin?74:PF_Err_NONE;}
PF_Err add(PF_ProgPtr ref,PF_ParamIndex,PF_ParamDef* value){auto& f=fixture(ref);f.params[++f.checkout]=*value;return PF_Err_NONE;}
PF_Err abort_cb(PF_ProgPtr ref){auto& f=fixture(ref);++f.abort_calls;return f.abort_now?75:PF_Err_NONE;}
SPErr acquire_cb(const char* name,int32 version,const void** suite){++active->acquire;if(active->fail_suite)return 76;if(std::strcmp(name,kPFWorldSuite)||version!=kPFWorldSuiteVersion2)return 77;*suite=&active->worlds;return 0;}
SPErr release_cb(const char*,int32){++active->release;return active->fail_release?78:0;}
PF_Err pixel_format(const PF_EffectWorld* world,PF_PixelFormat* format){if(active->fail_format)return 79;if(!world->width||!world->height)return 80;*format=static_cast<PF_PixelFormat>(reinterpret_cast<std::uintptr_t>(world->reserved0));return PF_Err_NONE;}
PF_Err checkout_pixels(PF_ProgPtr ref,A_long,PF_EffectWorld** world){auto& f=fixture(ref);++f.pixels_out;if(f.fail_pixel_checkout)return 81;*world=&f.input;return PF_Err_NONE;}
PF_Err checkout_output(PF_ProgPtr ref,PF_EffectWorld** world){auto& f=fixture(ref);if(f.fail_output)return 82;*world=&f.output;return PF_Err_NONE;}
PF_Err checkin_pixels(PF_ProgPtr ref,A_long){auto& f=fixture(ref);++f.pixels_in;return f.fail_pixel_checkin?83:PF_Err_NONE;}
Fixture::Fixture(){
    in.appl_id=kAppID_AfterEffects;
    in.effect_ref=reinterpret_cast<PF_ProgPtr>(this);in.time_scale=24;in.time_step=1;in.pica_basicP=&basic;
    in.inter.checkout_param=checkout_cb;in.inter.checkin_param=checkin_cb;in.inter.add_param=add;in.inter.abort=abort_cb;
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
    check(f.checkout==ParamCount-1&&f.checkin==f.checkout&&f.acquire==1&&f.release==1&&f.pixels_in==1,"Successful native resources unbalanced");
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
    check(run(unknown,true)==PF_Err_BAD_CALLBACK_PARAM&&unknown.acquire==0&&unknown.checkin==ParamCount-1&&unknown.pixels_in==1,"Unknown space error/resources wrong");
}
void failures(){
    for(int mode=0;mode<10;++mode){
        Fixture f;std::vector<PF_Pixel> input{{255,10,20,30}},output(1);
        set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB32);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB32);
        if(mode==0)f.fail_param=4;
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
        check(f.checkin==(f.fail_param?f.fail_param-1:(mode==2||mode==3?0:ParamCount-1)),"Parameter failure cleanup wrong");
        check(f.pixels_in==(f.fail_pixel_checkout?0:1),"Pixel failure cleanup wrong");
        check(f.release==(f.acquire&&!f.fail_suite?1:0),"Suite failure cleanup wrong");
    }
    Fixture empty;std::vector<PF_PixelFloat> input,output(1);set_world(empty.input,input,0,0,0,PF_PixelFormat_ARGB128);set_world(empty.output,output,1,1,1,PF_PixelFormat_ARGB128);
    check(run(empty,true)==PF_Err_NONE&&output[0].alpha==0,"Empty input must be transparent without querying null pixel format");
    empty.output.width=empty.output.height=0;check(run(empty,true)==PF_Err_NONE,"Empty output failed");
}
}
int main(int argc,char** argv){try{
    check(argc==2,"Provide the actual built plugin executable");
    auto module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);check(module!=nullptr,"Native plugin dlopen failed");
    effect_fn=reinterpret_cast<PF_FilterProc>(dlsym(module,"EffectMain"));check(effect_fn!=nullptr,"EffectMain export missing");
    Fixture setup;active=&setup;check(effect_fn(PF_Cmd_GLOBAL_SETUP,&setup.in,&setup.out,nullptr,nullptr,nullptr)==PF_Err_NONE,"Global setup failed");
    check(setup.out.out_flags==0x02000440&&setup.out.out_flags2==0x08001400&&setup.out.my_version==32769,"PiPL/runtime flags mismatch");
    Fixture ui;active=&ui;check(effect_fn(PF_Cmd_PARAMS_SETUP,&ui.in,&ui.out,nullptr,nullptr,nullptr)==PF_Err_NONE&&ui.out.num_params==ParamCount,"Native control schema failed");
    check(ui.params[Contrast].uu.id==3&&ui.params[WorkingSpace].uu.id==9&&ui.params[Temperature].uu.id==10&&ui.params[Tint].uu.id==11,"Persistent IDs changed with UI order");
    Fixture non_ae;non_ae.in.appl_id=0;check(effect_fn(PF_Cmd_GLOBAL_SETUP,&non_ae.in,&non_ae.out,nullptr,nullptr,nullptr)==PF_Err_INVALID_CALLBACK,"Non-AE host accepted");
    formats(PF_PixelFormat_ARGB32,PF_Pixel{127,51,75,99});
    formats(PF_PixelFormat_ARGB64,PF_Pixel16{12345,65535,7777,5123});
    formats(PF_PixelFormat_ARGB128,PF_PixelFloat{.5f,-.2f,2.0f,.1f});processing();failures();
    std::vector<std::future<bool>> tasks;
    for(int i=0;i<8;++i)tasks.push_back(std::async(std::launch::async,[]{Fixture f;std::vector<PF_PixelFloat> input{{1,.1f,.2f,.3f}},output(1);set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB128);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB128);f.params[Exposure].u.fs_d.value=1;return run(f,true)==PF_Err_NONE&&output[0].red==.2f&&f.checkin==ParamCount-1&&f.pixels_in==1;}));
    for(auto& t:tasks)check(t.get(),"Parallel native snapshot mismatch");
    check(effect_fn(PF_Cmd_GLOBAL_SETDOWN,&setup.in,&setup.out,nullptr,nullptr,nullptr)==PF_Err_NONE,"Global setdown failed");
    check(dlclose(module)==0,"Native module cleanup failed");
    std::cout<<"PASS "<<checks<<" owned native callback assertions; actual AE loader/MFR: NOT RUN\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<" assertion "<<checks<<"\n";return 1;}}
