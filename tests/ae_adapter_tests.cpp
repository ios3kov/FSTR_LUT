// Owned SDK-callback fixture. Exercises native adapter bytes; not the AE loader/runtime.
#include "plugin.hpp"
#include "fstr/sdr_prepare.hpp"
#include "AE_EffectUI.h"
#include "AE_EffectSuites.h"
#include <cstring>
#include <algorithm>
#include <cmath>
#include <future>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <dlfcn.h>
#include <fstream>
#include "scene_cache_fixture.hpp"
#include <cstdlib>
#include <unistd.h>
#include <map>

namespace {
int checks=0;
PF_FilterProc effect_fn=nullptr;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
struct Fixture;
thread_local Fixture* active=nullptr;
std::mutex handles_mutex;std::map<PF_Handle,A_u_longlong> handles;
PF_Handle new_handle(A_u_longlong n){auto slot=static_cast<void**>(std::malloc(sizeof(void*)));if(!slot)return nullptr;*slot=std::calloc(1,std::size_t(n));if(!*slot){std::free(slot);return nullptr;}auto h=reinterpret_cast<PF_Handle>(slot);std::lock_guard<std::mutex> lock(handles_mutex);handles[h]=n;return h;}
void* lock_handle(PF_Handle h){return h?*reinterpret_cast<void**>(h):nullptr;}
void unlock_handle(PF_Handle){}
void dispose_handle(PF_Handle h){if(!h)return;std::lock_guard<std::mutex> lock(handles_mutex);handles.erase(h);std::free(*reinterpret_cast<void**>(h));std::free(h);}
A_u_longlong handle_size(PF_Handle h){std::lock_guard<std::mutex> lock(handles_mutex);auto it=handles.find(h);return it==handles.end()?0:it->second;}
struct Fixture {
    PF_InData in{};PF_OutData out{};SPBasicSuite basic{};PF_WorldSuite2 worlds{};PF_UtilCallbacks utils{};
    std::array<PF_ParamDef,ParamCount> params{};
    PF_EffectWorld input{},output{};std::array<PF_EffectWorld*,5> temporal{};std::array<PF_EffectWorld*,3> preparation{};std::vector<std::pair<int,A_long>> layer_times;
    int checkout=0,checkin=0,acquire=0,release=0,pixels_out=0,pixels_in=0,abort_calls=0;
    std::vector<std::string> guids;bool fail_guid=false;
    PF_LRect maximum_rect{0,0,96,96};
    std::vector<int> layer_ids;std::vector<PF_LRect> requests;std::vector<PF_Field> fields;int fail_layer=-1,fail_pixel_id=-1;
    int fail_param=0;bool fail_suite=false,fail_output=false,fail_pixel_checkout=false,fail_format=false;
    bool fail_checkin=false,fail_pixel_checkin=false,fail_release=false,abort_now=false;
    PF_ParamUtilsSuite3 param_utils{};std::string popup_label;
    std::vector<PF_EffectWorld> scene_worlds;int source_id=0,abort_after_scan=-1;int revision=1,state_reads=0,scan_reads=0,anchor_reads=0;
    Fixture();
    ~Fixture(){if(params[LUTReference].param_type==PF_Param_ARBITRARY_DATA&&params[LUTReference].u.arb_d.dephault)dispose_handle(params[LUTReference].u.arb_d.dephault);}
};
Fixture& fixture(PF_ProgPtr ref){return *reinterpret_cast<Fixture*>(ref);}
PF_Err checkout_cb(PF_ProgPtr ref,PF_ParamIndex id,A_long time,A_long,A_u_long,PF_ParamDef* value){auto& f=fixture(ref);++f.checkout;if(id==AdvancedStart||id==AdvancedEnd)return 74;if(f.fail_param&&id==f.fail_param)return 73;*value=f.params[id];if(id==Input&&!f.scene_worlds.empty()){if(time<0||std::size_t(time)>=f.scene_worlds.size())return 84;value->u.ld=f.scene_worlds[std::size_t(time)];}return PF_Err_NONE;}
PF_Err checkin_cb(PF_ProgPtr ref,PF_ParamDef*){auto& f=fixture(ref);++f.checkin;return f.fail_checkin?74:PF_Err_NONE;}
PF_Err add(PF_ProgPtr ref,PF_ParamIndex,PF_ParamDef* value){auto& f=fixture(ref);f.params[++f.checkout]=*value;return PF_Err_NONE;}
PF_Err register_ui_cb(PF_ProgPtr,PF_CustomUIInfo* value){return value&&value->events==PF_CustomEFlag_EFFECT?PF_Err_NONE:PF_Err_BAD_CALLBACK_PARAM;}
PF_Err abort_cb(PF_ProgPtr ref){auto& f=fixture(ref);++f.abort_calls;return (f.abort_now||(f.abort_after_scan>=0&&f.scan_reads>=f.abort_after_scan))?75:PF_Err_NONE;}
SPErr acquire_cb(const char* name,int32 version,const void** suite){++active->acquire;if(active->fail_suite)return 76;if(!std::strcmp(name,kPFParamUtilsSuite)&&version==kPFParamUtilsSuiteVersion3){*suite=&active->param_utils;return 0;}if(std::strcmp(name,kPFWorldSuite)||version!=kPFWorldSuiteVersion2)return 77;*suite=&active->worlds;return 0;}
SPErr release_cb(const char*,int32){++active->release;return active->fail_release?78:0;}
PF_Err pixel_format(const PF_EffectWorld* world,PF_PixelFormat* format){if(active->fail_format)return 79;if(!world->width||!world->height)return 80;*format=static_cast<PF_PixelFormat>(reinterpret_cast<std::uintptr_t>(world->reserved0));return PF_Err_NONE;}
PF_Err checkout_pixels(PF_ProgPtr ref,A_long id,PF_EffectWorld** world){auto& f=fixture(ref);++f.pixels_out;if(f.fail_pixel_checkout||id==f.fail_pixel_id)return 81;if(id>=100000){const auto frame=std::size_t((id-100000)/4);if(frame>=f.scene_worlds.size())return 84;if((id-100000)%4==0)++f.scan_reads;else if((id-100000)%4==2)++f.anchor_reads;else return 84;*world=&f.scene_worlds[frame];return 0;}*world=id>=2000&&id<2003&&f.preparation[id-2000]?f.preparation[id-2000]:(id>=0&&id<5&&f.temporal[id]?f.temporal[id]:&f.input);return PF_Err_NONE;}
PF_Err checkout_output(PF_ProgPtr ref,PF_EffectWorld** world){auto& f=fixture(ref);if(f.fail_output)return 82;*world=&f.output;return PF_Err_NONE;}
PF_Err checkin_pixels(PF_ProgPtr ref,A_long){auto& f=fixture(ref);++f.pixels_in;return f.fail_pixel_checkin?83:PF_Err_NONE;}
PF_Err update_ui_cb(PF_ProgPtr ref,PF_ParamIndex id,const PF_ParamDef* ui){auto& f=fixture(ref);if(id!=Look||!ui||ui->param_type!=PF_Param_POPUP)return PF_Err_BAD_CALLBACK_PARAM;f.popup_label=ui->u.pd.u.namesptr;return 0;}
PF_Err current_state(PF_ProgPtr ref,PF_ParamIndex id,const A_Time* start,const A_Time* duration,PF_State* state){auto&f=fixture(ref);++f.state_reads;if(id!=Input||start||duration)return 87;*state={};state->reservedAL[0]=f.revision;state->reservedAL[1]=f.source_id;return 0;}
PF_Err same_state(PF_ProgPtr,const PF_State*a,const PF_State*b,A_Boolean*same){*same=std::memcmp(a,b,sizeof(*a))==0;return 0;}
Fixture::Fixture(){
    static std::atomic<int> identity{1};source_id=identity.fetch_add(1);
    param_utils.PF_GetCurrentState=current_state;param_utils.PF_AreStatesIdentical=same_state;
    param_utils.PF_UpdateParamUI=update_ui_cb;
    in.utils=&utils;utils.host_new_handle=new_handle;utils.host_lock_handle=lock_handle;utils.host_unlock_handle=unlock_handle;utils.host_dispose_handle=dispose_handle;utils.host_get_handle_size=handle_size;
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
    const PF_OutFlags first[]{0,0x06008042,PF_OutFlag_WIDE_TIME_INPUT,PF_OutFlag_PIX_INDEPENDENT};
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
    check(ui.params[Look].u.pd.num_choices==19&&ui.params[Look].uu.id==6,"Cinema/Custom menu or persistent ID wrong");
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
PF_Err checkout_layer(PF_ProgPtr ref,PF_ParamIndex,A_long id,const PF_RenderRequest* request,A_long time,A_long,A_u_long,PF_CheckoutResult* result){
 auto&f=fixture(ref);f.layer_ids.push_back(id);f.layer_times.emplace_back(id,time);f.requests.push_back(request->rect);f.fields.push_back(request->field);if(id==f.fail_layer)return 85;
 result->result_rect=request->rect;result->max_result_rect=f.maximum_rect;return PF_Err_NONE;
}
PF_Err mix_guid(PF_ProgPtr,A_u_long,const void*);
PF_Err declared_render(Fixture& f){
 active=&f;PF_PreRenderInput pi{};pi.bitdepth=32;pi.output_request.field=f.in.field;pi.output_request.rect={f.output.origin_x,f.output.origin_y,f.output.origin_x+f.output.width,f.output.origin_y+f.output.height};
 PF_PreRenderOutput po{};PF_PreRenderCallbacks pc{};pc.checkout_layer=checkout_layer;pc.GuidMixInPtr=mix_guid;PF_PreRenderExtra pe{&pi,&po,&pc};
 auto err=effect_fn(PF_Cmd_SMART_PRE_RENDER,&f.in,&f.out,nullptr,nullptr,&pe);if(err)return err;
 PF_SmartRenderInput ri{};ri.pre_render_data=po.pre_render_data;PF_SmartRenderCallbacks rc{};rc.checkout_layer_pixels=checkout_pixels;rc.checkout_output=checkout_output;rc.checkin_layer_pixels=checkin_pixels;
 PF_SmartRenderExtra re{};re.input=&ri;re.cb=&rc;err=effect_fn(PF_Cmd_SMART_RENDER,&f.in,&f.out,nullptr,nullptr,&re);if(po.pre_render_data)po.delete_pre_render_data_func(po.pre_render_data);return err;
}
template<class P>void prepared_integer(PF_PixelFormat format,double maximum){
 Fixture f;f.in.total_time=72;f.params[AutoPrepare].u.bd.value=TRUE;f.maximum_rect={0,0,96,1};std::vector<P> input(96),output(97);std::vector<float> rgba;
 for(int x=0;x<96;++x){const auto v=static_cast<decltype(P::red)>((.04+.36*x/95)*maximum+.5);input[x]={static_cast<decltype(P::alpha)>(maximum),v,v,v};rgba.insert(rgba.end(),{float(v/maximum),float(v/maximum),float(v/maximum),1});}
 set_world(f.input,input,96,1,96,format);set_world(f.output,output,96,1,97,format);for(auto& w:f.preparation)w=&f.input;
 auto recipe=fstr::analyze_sdr_shot({{rgba.data(),rgba.size(),96,1,96*4,fstr::Transfer::Linear}}).settings;check(declared_render(f)==0,"Integer automatic preparation failed");
 for(int x=0;x<96;++x){auto c=fstr::prepare_sdr_linear({rgba[x*4],rgba[x*4+1],rgba[x*4+2]},recipe);check(std::abs(double(output[x].red)-std::floor(c.r*maximum+.5))<=1&&output[x].alpha==maximum,"Integer auto preparation scale differs from core");}
}
void scene_timeline_callbacks(){
 Fixture f;const int w=96,h=54,n=6;std::vector<std::vector<PF_PixelFloat>> pixels(n,std::vector<PF_PixelFloat>(w*h));std::vector<PF_PixelFloat> output(w*h);f.scene_worlds.resize(n);
 for(int k=0;k<n;++k){for(int y=0;y<h;++y)for(int x=0;x<w;++x){float v=.04f+.36f*float(x)/float(w-1);pixels[k][y*w+x]=k<3?PF_PixelFloat{1,v,v,v}:PF_PixelFloat{1,v*.2f,v*.4f,v};}set_world(f.scene_worlds[k],pixels[k],w,h,w,PF_PixelFormat_ARGB128);}
 f.input=f.scene_worlds[0];set_world(f.output,output,w,h,w,PF_PixelFormat_ARGB128);f.maximum_rect={0,0,w,h};f.in.width=w;f.in.height=h;f.in.total_time=n;f.params[AutoPrepare].u.bd.value=TRUE;f.params[DetectSceneChanges].u.bd.value=TRUE;
 auto close=[&]{active=&f;check(effect_fn(PF_Cmd_SEQUENCE_SETDOWN,&f.in,&f.out,nullptr,nullptr,nullptr)==0,"Scene sequence cleanup failed");};
 auto reference=[&](int k){std::vector<float> rgba;for(auto&p:pixels[k])rgba.insert(rgba.end(),{p.red,p.green,p.blue,p.alpha});return fstr::analyze_sdr_shot({{rgba.data(),rgba.size(),w,h,w*4,fstr::Transfer::Linear}}).settings;};
 auto verify=[&](int k){auto settings=reference(k);for(int x=0;x<w;++x){auto&p=pixels[k][x];auto v=fstr::prepare_sdr_linear({p.red,p.green,p.blue},settings);check(std::abs(output[x].red-v.r)<2e-6&&std::abs(output[x].blue-v.b)<2e-6,"Native selected wrong shot recipe");}};
 check(declared_render(f)==0,"Native complete timeline failed");check(f.scan_reads==n&&f.anchor_reads==n,"Native timeline did not read unique scan/anchor frames");verify(0);int scans=f.scan_reads,anchors=f.anchor_reads;
 for(int k:{5,3,0,4}){f.in.current_time=k;f.input=f.scene_worlds[k];auto declarations=f.layer_ids.size();check(declared_render(f)==0,"Out-of-order scene render failed");verify(k);check(f.scan_reads==scans&&f.anchor_reads==anchors,"Warm request rescanned timeline");check(f.layer_ids.size()-declarations>=std::size_t(n*3),"Warm cache omitted all-time dependencies");}
 f.params[LookAmount].u.fs_d.value=120;check(declared_render(f)==0&&f.scan_reads==scans,"Strength edit recomputed scene analysis");
 Fixture clone;clone.source_id=f.source_id;clone.scene_worlds=f.scene_worlds;clone.in.width=w;clone.in.height=h;clone.in.total_time=n;clone.maximum_rect=f.maximum_rect;clone.params=f.params;clone.input=f.input;clone.output=f.output;check(declared_render(clone)==0&&clone.scan_reads==0&&clone.anchor_reads==0,"AE render clone did not reuse identical input state");
 ++f.revision;check(declared_render(f)==0&&f.scan_reads==scans+n,"Upstream state change retained stale timeline");scans=f.scan_reads;
 f.in.pixel_aspect_ratio={2,1};check(declared_render(f)==0&&f.scan_reads==scans+n,"Geometry change retained stale timeline");scans=f.scan_reads;
 ++f.revision;f.fail_pixel_id=100004;auto out_before=f.pixels_out,in_before=f.pixels_in;check(declared_render(f)==81,"Scan failure swallowed");check(f.pixels_out-out_before==f.pixels_in-in_before+1,"Scan failure leaked a checked-out world");f.fail_pixel_id=-1;scans=f.scan_reads;check(declared_render(f)==0&&f.scan_reads==scans+n,"Failed partial analysis was published");
 ++f.revision;f.fail_pixel_id=100002;out_before=f.pixels_out;in_before=f.pixels_in;check(declared_render(f)==81,"Anchor failure swallowed");check(f.pixels_out-out_before==f.pixels_in-in_before+1,"Anchor failure leaked input");f.fail_pixel_id=-1;scans=f.scan_reads;check(declared_render(f)==0&&f.scan_reads==scans+n,"Failed anchor recipe was published");
 ++f.revision;f.abort_after_scan=f.scan_reads+3;out_before=f.pixels_out;in_before=f.pixels_in;check(declared_render(f)==75,"Mid-scan cancellation ignored");check(f.pixels_out-out_before==f.pixels_in-in_before,"Cancelled scan leaked input");f.abort_after_scan=-1;scans=f.scan_reads;check(declared_render(f)==0&&f.scan_reads==scans+n,"Cancelled timeline was published");
 close();scans=f.scan_reads;auto legacy_out=f.checkout,legacy_in=f.checkin;check(run(f,false)==0,"Legacy scene path failed");verify(4);check(f.checkout-legacy_out==f.checkin-legacy_in,"Legacy scene checkouts unbalanced");close();
 f.params[Mix].u.fs_d.value=0;auto states=f.state_reads;check(declared_render(f)==0&&f.state_reads==states,"Mix zero performed scene analysis");f.params[Mix].u.fs_d.value=100;f.params[AutoPrepare].u.bd.value=FALSE;check(declared_render(f)==0&&f.state_reads==states,"Auto Off performed scene analysis");f.params[AutoPrepare].u.bd.value=TRUE;f.params[Source].u.pd.value=3;check(declared_render(f)==0&&f.state_reads==states,"Explicit Log performed SDR scene analysis");f.params[Source].u.pd.value=1;
 f.params[WorkingSpace].u.pd.value=1;states=f.state_reads;check(declared_render(f)!=0&&f.state_reads==states,"Unspecified encoding triggered scan");f.params[WorkingSpace].u.pd.value=3;
 f.in.field=PF_Field_UPPER;auto begin=f.layer_ids.size();check(declared_render(f)==0,"Field output scene analysis failed");for(auto i=begin;i<f.layer_ids.size();++i)if(f.layer_ids[i]>=100000)check(f.fields[i]==PF_Field_FRAME,"Scene scan inherited partial-field request");check(run(f,false)!=0,"Legacy partial-field scene analysis accepted");f.in.field=PF_Field_FRAME;
 f.in.total_time=16385;check(declared_render(f)!=0&&std::strstr(f.out.return_msg,"too long"),"Over-budget clip was not actionable");close();
 Fixture schema;active=&schema;check(effect_fn(PF_Cmd_PARAMS_SETUP,&schema.in,&schema.out,nullptr,nullptr,nullptr)==0,"Scene schema failed");const auto&p=schema.params[DetectSceneChanges];check(p.uu.id==21&&p.u.bd.dephault==TRUE&&p.u.bd.value==FALSE&&(p.flags&PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS)&&(p.flags&PF_ParamFlag_CANNOT_TIME_VARY),"Scene default/legacy ID contract wrong");
}
void custom_lut_reference(){
 Fixture f;active=&f;const auto before=handles.size();std::vector<PF_ArbitraryH> owned;
 struct Release {Fixture& f;std::vector<PF_ArbitraryH>& hs;~Release(){for(auto h:hs){PF_ArbParamsExtra e{};e.id=ExternalLUTId;e.which_function=PF_Arbitrary_DISPOSE_FUNC;e.u.dispose_func_params.arbH=h;effect_fn(PF_Cmd_ARBITRARY_CALLBACK,&f.in,&f.out,nullptr,nullptr,&e);}}} release{f,owned};
 auto call=[&](PF_ArbParamsExtra& e){e.id=ExternalLUTId;return effect_fn(PF_Cmd_ARBITRARY_CALLBACK,&f.in,&f.out,nullptr,nullptr,&e);};
 auto flatten=[&](PF_ArbitraryH h){PF_ArbParamsExtra e{};A_u_long n=0;e.which_function=PF_Arbitrary_FLAT_SIZE_FUNC;e.u.flat_size_func_params.arbH=h;e.u.flat_size_func_params.flat_data_sizePLu=&n;check(call(e)==0&&n<32768,"LUT flat size invalid");std::string s(n,' ');e={};e.which_function=PF_Arbitrary_FLATTEN_FUNC;e.u.flatten_func_params.arbH=h;e.u.flatten_func_params.buf_sizeLu=n;e.u.flatten_func_params.flat_dataPV=s.data();check(call(e)==0,"LUT flatten failed");return s;};
 auto scan=[&](const std::string& s){PF_ArbitraryH h=nullptr;PF_ArbParamsExtra e{};e.which_function=PF_Arbitrary_SCAN_FUNC;e.u.scan_func_params.bufPC=s.data();e.u.scan_func_params.bytes_to_scanLu=static_cast<A_u_long>(s.size());e.u.scan_func_params.arbPH=&h;check(call(e)==0&&h,"LUT reference scan failed");owned.push_back(h);return h;};
 PF_ArbitraryH empty=nullptr;PF_ArbParamsExtra e{};e.which_function=PF_Arbitrary_NEW_FUNC;e.u.new_func_params.arbPH=&empty;check(call(e)==0&&empty,"Default external reference allocation failed");owned.push_back(empty);auto flat_empty=flatten(empty);check(nlohmann::json::parse(flat_empty).at("path")=="","Default reference nonempty");scan(flat_empty);
 const auto folder=std::filesystem::temp_directory_path()/("fstr-custom-reference-"+std::to_string(::getpid()));std::filesystem::create_directories(folder);struct Clean{std::filesystem::path p;~Clean(){std::error_code e;std::filesystem::remove_all(p,e);}}clean{folder};const auto path=folder/"Проверка LUT.cube";
 std::string cube="LUT_3D_SIZE 2\n";for(int b=0;b<2;++b)for(int g=0;g<2;++g)for(int r=0;r<2;++r)cube+=std::to_string(g)+" "+std::to_string(b)+" "+std::to_string(r)+"\n";
 {std::ofstream out(path);out<<cube;}
 const auto serialized=nlohmann::json{{"schema",1},{"path",path.string()},{"fingerprint",""}}.dump();auto loaded=scan(serialized);const auto flat=flatten(loaded);check(flat.size()<1024&&flat.find("LUT_3D_SIZE")==std::string::npos&&nlohmann::json::parse(flat).at("path")==path.string(),"Project embeds LUT source rather than reference");
 PF_ArbitraryH copied=nullptr;e={};e.which_function=PF_Arbitrary_COPY_FUNC;e.u.copy_func_params.src_arbH=loaded;e.u.copy_func_params.dst_arbPH=&copied;check(call(e)==0&&copied!=loaded,"Reference copy failed/aliased host handle");owned.push_back(copied);
 e={};e.which_function=PF_Arbitrary_COMPARE_FUNC;PF_ArbCompareResult comparison=-1;e.u.compare_func_params.a_arbH=loaded;e.u.compare_func_params.b_arbH=copied;e.u.compare_func_params.compareP=&comparison;check(call(e)==0&&comparison==PF_ArbCompare_EQUAL,"Copied reference differs");
 std::vector<PF_PixelFloat> input{{1,.2f,.4f,.7f}},output(1);set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB128);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB128);f.params[Look].u.pd.value=18;f.params[LookAmount].u.fs_d.value=100;f.params[LUTReference].u.arb_d.value=copied;f.params[Look].param_type=PF_Param_POPUP;PF_ParamDef* ui_params[ParamCount];for(int i=0;i<ParamCount;++i)ui_params[i]=&f.params[i];active=&f;check(effect_fn(PF_Cmd_UPDATE_PARAMS_UI,&f.in,&f.out,ui_params,nullptr,nullptr)==0&&f.popup_label.find("Proverka LUT.cube")!=std::string::npos,"Selected external LUT filename missing from menu");
PF_UserChangedParamExtra selected{};selected.param_index=Look;check(effect_fn(PF_Cmd_USER_CHANGED_PARAM,&f.in,&f.out,ui_params,nullptr,&selected)==0&&f.params[Look].u.pd.value==18&&flatten(copied)==flat,"Selecting stored Custom reopened chooser or changed reference");
 check(f.popup_label.size()>=12&&f.popup_label.substr(f.popup_label.size()-12)=="|Load LUT...","Reload action is not last after named Custom");
#ifdef FSTR_TEST_EXTERNAL_LUT
 check(declared_render(f)==0&&std::abs(output[0].red-.4f)<3e-6&&std::abs(output[0].green-.7f)<3e-6&&std::abs(output[0].blue-.2f)<3e-6,"Custom LUT native rendering failed");auto original_guid=f.guids.back();
#endif
 std::filesystem::remove(path);
#ifdef FSTR_TEST_EXTERNAL_LUT
 check(declared_render(f)==0&&std::abs(output[0].red-.4f)<3e-6,"Existing resource lost after source removal");
#endif
 auto missing=scan(flat);f.params[LUTReference].u.arb_d.value=missing;active=&f;check(effect_fn(PF_Cmd_UPDATE_PARAMS_UI,&f.in,&f.out,ui_params,nullptr,nullptr)==0&&f.popup_label.find("|Missing: ")!=std::string::npos,"Missing-file menu label not explicit");check(declared_render(f)!=0,"Reopened missing LUT silently succeeded");f.params[Mix].u.fs_d.value=0;check(declared_render(f)==0&&std::memcmp(input.data(),output.data(),sizeof(PF_PixelFloat))==0,"Custom missing-file Mix0 not exact/no-analysis");f.params[Mix].u.fs_d.value=100;
#ifdef FSTR_TEST_EXTERNAL_LUT
 {std::ofstream out(path);out<<"LUT_3D_SIZE 2\n";for(int b=0;b<2;++b)for(int g=0;g<2;++g)for(int r=0;r<2;++r)out<<r<<' '<<g<<' '<<b<<'\n';}auto revised=scan(flat);f.params[LUTReference].u.arb_d.value=revised;check(declared_render(f)==0&&std::abs(output[0].red-.2f)<3e-6&&f.guids.back()!=original_guid,"Changed external file reused old mapping/host GUID");
#endif
 PF_ArbitraryH invalid=nullptr;e={};e.which_function=PF_Arbitrary_SCAN_FUNC;const std::string bad="{\"schema\":2}";e.u.scan_func_params.bufPC=bad.data();e.u.scan_func_params.bytes_to_scanLu=static_cast<A_u_long>(bad.size());e.u.scan_func_params.arbPH=&invalid;check(call(e)!=0&&!invalid,"Invalid reference schema accepted");
 for(auto h:owned){e={};e.which_function=PF_Arbitrary_DISPOSE_FUNC;e.u.dispose_func_params.arbH=h;check(call(e)==0,"LUT handle disposal failed");}owned.clear();check(handles.size()==before,"Arbitrary reference handles leaked");
}

void automatic_preparation(){
 prepared_integer<PF_Pixel>(PF_PixelFormat_ARGB32,PF_MAX_CHAN8);prepared_integer<PF_Pixel16>(PF_PixelFormat_ARGB64,PF_MAX_CHAN16);
 Fixture f;f.in.total_time=72;f.in.current_time=22;f.params[AutoPrepare].u.bd.value=TRUE;f.maximum_rect={0,0,96,1};
 std::vector<PF_PixelFloat> input,output(96);std::array<std::vector<PF_PixelFloat>,3> pixels;std::array<PF_EffectWorld,3> worlds;std::array<std::vector<float>,3> rgba;
 for(int x=0;x<96;++x){float c=float(.04+.36*x/95);input.push_back({1,c,c,c});for(int a=0;a<3;++a){float v=c+float(a)*.001f;pixels[a].push_back({1,v,v,v});rgba[a].insert(rgba[a].end(),{v,v,v,1});}}
 set_world(f.input,input,96,1,96,PF_PixelFormat_ARGB128);set_world(f.output,output,96,1,96,PF_PixelFormat_ARGB128);
 std::vector<fstr::FrameView> views;for(int i=0;i<3;++i){set_world(worlds[i],pixels[i],96,1,96,PF_PixelFormat_ARGB128);f.preparation[i]=&worlds[i];views.push_back({rgba[i].data(),rgba[i].size(),96,1,96*4,fstr::Transfer::Linear});}
 auto recipe=fstr::analyze_sdr_shot(views).settings;check(declared_render(f)==0,"Native automatic clip preparation failed");
 auto value=[&](int x){return fstr::prepare_sdr_linear({input[x].red,input[x].green,input[x].blue},recipe);};
 for(int x=0;x<96;++x)check(std::abs(output[x].red-value(x).r)<3e-7&&output[x].alpha==1,"Native preparation differs from standalone core");
 for(auto expected:std::vector<std::pair<int,A_long>>{{2000,0},{2001,35},{2002,71}})check(std::find(f.layer_times.begin(),f.layer_times.end(),expected)!=f.layer_times.end(),"Wrong fixed clip anchor checkout");
 check(f.pixels_out==f.pixels_in&&f.checkin==RenderParamCount,"Prepared native resources unbalanced");auto complete=output;const auto first_guid=f.guids.back();
 f.layer_times.clear();f.in.current_time=50;check(declared_render(f)==0&&std::memcmp(output.data(),complete.data(),output.size()*sizeof(PF_PixelFloat))==0,"Current request time changed clip preparation");
 check(f.guids.back()!=first_guid,"Host cache aliases different output times with fixed anchor dependencies");
 f.params[Mix].u.fs_d.value=50;check(declared_render(f)==0,"Prepared Mix50 failed");for(int x=0;x<96;++x)check(std::abs(output[x].red-(value(x).r+input[x].red)*.5)<3e-7,"Mix did not include preparation");
 f.params[Mix].u.fs_d.value=0;f.fail_pixel_id=2000;check(declared_render(f)==0&&std::memcmp(input.data(),output.data(),input.size()*sizeof(PF_PixelFloat))==0,"Prepared Mix0 checked anchors or changed pixels");f.fail_pixel_id=-1;f.params[Mix].u.fs_d.value=100;
 std::vector<PF_PixelFloat> roi(2);roi[1]={.1f,.2f,.3f,.4f};auto guard=roi[1];set_world(f.output,roi,1,1,2,PF_PixelFormat_ARGB128,25,0);check(declared_render(f)==0&&std::abs(roi[0].red-value(25).r)<3e-7&&std::memcmp(&roi[1],&guard,sizeof(guard))==0,"Auto preparation changed ROI or padding");
 set_world(f.output,output,96,1,96,PF_PixelFormat_ARGB128);auto before_out=f.pixels_out,before_in=f.pixels_in;f.fail_pixel_id=2001;check(declared_render(f)!=0&&f.pixels_out-before_out==f.pixels_in-before_in+1,"Anchor checkout failure leaked earlier worlds");f.fail_pixel_id=-1;
 for(int profile:{2,static_cast<int>(fstr::source_profile_count)}){f.params[Source].u.pd.value=profile;f.fail_pixel_id=2000;check(declared_render(f)==0,"Default SDR preparation blocked non-SDR input");const auto non_sdr=output;f.params[AutoPrepare].u.bd.value=FALSE;check(declared_render(f)==0&&std::memcmp(output.data(),non_sdr.data(),output.size()*sizeof(PF_PixelFloat))==0,"SDR preparation changed Log/3D output");f.params[AutoPrepare].u.bd.value=TRUE;}f.fail_pixel_id=-1;f.params[Source].u.pd.value=1;
 Fixture legacy;legacy.params[AutoPrepare].u.bd.value=TRUE;legacy.in.total_time=72;legacy.input=f.input;legacy.output=f.output;check(run(legacy,false)==0&&legacy.checkin==RenderParamCount+3,"Legacy timed preparation checkouts unbalanced");
 Fixture schema;active=&schema;check(effect_fn(PF_Cmd_PARAMS_SETUP,&schema.in,&schema.out,nullptr,nullptr,nullptr)==0&&schema.params[AutoPrepare].uu.id==17&&schema.params[AutoPrepare].u.bd.dephault==TRUE&&schema.params[AutoPrepare].u.bd.value==FALSE&&(schema.params[AutoPrepare].flags&PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS)&&AutoPrepare==1&&CameraInfo==18,"Auto-first default or legacy value policy changed");
 check(schema.params[Exposure].uu.id==2&&schema.params[Look].uu.id==6&&schema.params[ImportLUT].uu.id==18&&schema.params[ImportLUT].param_type==PF_Param_BUTTON&&(schema.params[ImportLUT].ui_flags&PF_PUI_NO_ECW_UI),"Legacy disk IDs/hidden former button changed");
 check((schema.params[Look].flags&PF_ParamFlag_SUPERVISE)&&std::string(schema.params[Look].u.pd.u.namesptr).find("|Custom|Load LUT...")!=std::string::npos,"Custom load entry not supervised/last");check(schema.params[LUTFileInfo].ui_flags&PF_PUI_NO_ECW_UI,"Separate LUT filename block is still visible");check(schema.popup_label.empty(),"Setup unexpectedly mutated UI");
 PF_ParamDef* ui[ParamCount];for(int i=0;i<ParamCount;++i)ui[i]=&schema.params[i];schema.params[Look].u.pd.value=3;check(effect_fn(PF_Cmd_UPDATE_PARAMS_UI,&schema.in,&schema.out,ui,nullptr,nullptr)==0,"UI snapshot failed");PF_UserChangedParamExtra changed{};changed.param_index=Look;schema.params[Look].u.pd.value=4;check(effect_fn(PF_Cmd_USER_CHANGED_PARAM,&schema.in,&schema.out,ui,nullptr,&changed)==0&&schema.params[Look].u.pd.value==4,"Built-in selection incorrectly opened chooser");

}
void smart_requests(){
 for(int mode=0;mode<6;++mode){
  Fixture f;active=&f;f.in.current_time=10;f.in.total_time=100;
  f.params[SmartColor].u.bd.value=mode!=0;f.params[Look].u.pd.value=3;
  PF_PreRenderInput pi{};pi.bitdepth=32;pi.output_request.field=f.in.field;pi.output_request.rect={10,10,20,20};PF_PreRenderOutput po{};PF_PreRenderCallbacks pc{};pc.checkout_layer=checkout_layer;pc.GuidMixInPtr=mix_guid;
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
  f.params[SmartColor].u.bd.value=TRUE;active=&f;PF_PreRenderInput pi{};pi.bitdepth=32;pi.output_request.field=f.in.field;pi.output_request.rect={0,0,w,h};PF_PreRenderCallbacks pc{};pc.checkout_layer=checkout_layer;pc.GuidMixInPtr=mix_guid;PF_PreRenderExtra pe{&pi,&po,&pc};check(effect_fn(PF_Cmd_SMART_PRE_RENDER,&f.in,&f.out,nullptr,nullptr,&pe)==0,"Automatic explicit pre-render failed");si.pre_render_data=po.pre_render_data;
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
 PF_PreRenderInput pi{};pi.bitdepth=32;pi.output_request.field=f.in.field;pi.output_request.rect={0,0,width,height};PF_PreRenderOutput po{};PF_PreRenderCallbacks pc{};pc.checkout_layer=checkout_layer;pc.GuidMixInPtr=mix_guid;PF_PreRenderExtra pe{&pi,&po,&pc};check(effect_fn(PF_Cmd_SMART_PRE_RENDER,&f.in,&f.out,nullptr,nullptr,&pe)==0,"Recovery pre-render failed");
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

std::atomic<int> host_iteration_calls{0};
PF_Err parallel_host(A_long rows,void* ref,PF_Err(*fn)(void*,A_long,A_long,A_long)){
    ++host_iteration_calls;std::atomic<A_long> next{0};std::atomic<PF_Err> error{0};
    auto work=[&](A_long t){while(!error){auto y=next.fetch_add(1);if(y>=rows)break;auto e=fn(ref,t,y,rows);if(e)error=e;}};
    std::thread a(work,1),b(work,2);work(0);a.join();b.join();return error;
}
template<class P>void host_rows(PF_PixelFormat format,P pixel){
    Fixture serial,parallel;std::vector<P> input(256*256,pixel),expected(input.size()),actual(input.size());
    for(auto* f:{&serial,&parallel}){f->params[Look].u.pd.value=9;f->params[Exposure].u.fs_d.value=.5;f->params[Temperature].u.fs_d.value=20;f->params[Tint].u.fs_d.value=-10;set_world(f->input,input,256,256,256,format);}
    set_world(serial.output,expected,256,256,256,format);set_world(parallel.output,actual,256,256,256,format);parallel.utils.iterate_generic=parallel_host;
    check(run(serial,true)==0&&run(parallel,true)==0,"Host row rendering failed");check(std::memcmp(expected.data(),actual.data(),actual.size()*sizeof(P))==0,"Host iterator pixels differ from serial");check(host_iteration_calls>0,"Host iterator not executed");
    set_world(serial.input,expected,256,256,256,format);set_world(parallel.input,actual,256,256,256,format);const auto calls=host_iteration_calls.load();
    check(run(serial,true)==0&&run(parallel,true)==0&&std::memcmp(expected.data(),actual.data(),actual.size()*sizeof(P))==0,"In-place serial fallback changed pixels");check(host_iteration_calls==calls,"Aliased input/output used host workers");
    parallel.abort_now=true;check(run(parallel,true)==75,"Host row initial cancellation ignored");
}

void smart_host_rows(){
 char pattern[]="/tmp/fstr-parallel-smart-XXXXXX";auto name=mkdtemp(pattern);check(name!=nullptr,"Parallel Smart temp");const std::filesystem::path root(name);
 struct Guard{std::filesystem::path p;~Guard(){unsetenv("FSTR_LUT_SCENE_CACHE");unsetenv("FSTR_LUT_SCENE_POLICY");std::error_code e;std::filesystem::remove_all(p,e);}} guard{root};
 const int n=256;fstr::AnalysisImage image{n,n,{}};image.rgba.resize(n*n*4);std::vector<PF_PixelFloat> input(n*n),a(n*n),b(n*n);
 for(int y=0;y<n;++y)for(int x=0;x<n;++x){const auto i=y*n+x;auto*rgb=&image.rgba[4*i];const float alpha=x%31==0?0:.25f+.75f*y/255;
  rgb[0]=alpha?120:0;rgb[1]=alpha?70:0;rgb[2]=alpha?40:0;rgb[3]=255;input[i]={alpha,alpha*120.f/255,alpha*70.f/255,alpha*40.f/255};}
 const std::string policy(64,'b');const auto key=scene_fixture::write(root,image,n,policy,true);
 auto payload=[&](const std::string&path,const std::vector<float>&v){std::string bytes(v.size()*4,'\0');for(std::size_t i=0;i<v.size();++i){std::uint32_t bits;std::memcpy(&bits,&v[i],4);for(int c=0;c<4;++c)bytes[4*i+c]=char((bits>>(c*8))&255);}std::ofstream f(root/path,std::ios::binary);f.write(bytes.data(),bytes.size());f.close();return picosha2::hash256_hex_string(bytes);};
 nlohmann::json j;{std::ifstream f(root/(key+".fstrscene"));f>>j;}
 j["people"].push_back(j["people"][0]);std::vector<float> mask(n*n),support(n*n);
 for(int person=0;person<2;++person){for(int y=0;y<n;++y)for(int x=0;x<n;++x){auto i=y*n+x;double v=double(x)/255;mask[i]=float(person?v:1-v);support[i]=float(mask[i]*(double(y)/255));}
  const auto stem=key+"-p"+std::to_string(person);j["people"][person]["mask_sha256"]=payload(stem+".mask",mask);j["people"][person]["toning_support_sha256"]=payload(stem+".support",support);j["people"][person]["toning_strength"]=person?.8:.3;
  if(person){std::vector<float> samples;for(int i=0;i<40;++i)for(float v:{.45f,.27f,.18f,1.f})samples.push_back(v);j["people"][person]["samples_sha256"]=payload(stem+".samples",samples);}
 }
 {std::ofstream f(root/(key+".fstrscene"));f<<j.dump();}
 setenv("FSTR_LUT_SCENE_CACHE",root.c_str(),1);setenv("FSTR_LUT_SCENE_POLICY",policy.c_str(),1);Fixture setup;active=&setup;check(effect_fn(PF_Cmd_GLOBAL_SETUP,&setup.in,&setup.out,nullptr,nullptr,nullptr)==0,"Parallel Smart global setup");
 Fixture serial,parallel;for(auto*f:{&serial,&parallel}){set_world(f->input,input,n,n,n,PF_PixelFormat_ARGB128);f->params[WorkingSpace].u.pd.value=2;f->params[Look].u.pd.value=9;f->params[SmartColor].u.bd.value=TRUE;}
 set_world(serial.output,a,n,n,n,PF_PixelFormat_ARGB128);set_world(parallel.output,b,n,n,n,PF_PixelFormat_ARGB128);parallel.utils.iterate_generic=parallel_host;const auto before=host_iteration_calls.load();
 check(run(serial,true)==0&&run(parallel,true)==0,"Parallel two-person Smart failed");check(host_iteration_calls>before,"Smart failed to execute host iterator");check(std::memcmp(a.data(),b.data(),a.size()*sizeof(PF_PixelFloat))==0,"Smart people/support/alpha parallel mismatch");
 serial.params[SmartColor].u.bd.value=FALSE;check(run(serial,true)==0&&std::memcmp(a.data(),b.data(),a.size()*sizeof(PF_PixelFloat))!=0,"Smart oracle had no correction");
 active=&setup;check(effect_fn(PF_Cmd_GLOBAL_SETDOWN,&setup.in,&setup.out,nullptr,nullptr,nullptr)==0,"Parallel Smart setdown");
}
void* cleanup_module=nullptr;
int main(int argc,char** argv){const auto prep_root=std::filesystem::temp_directory_path()/("fstr-native-prep-owned-"+std::to_string(getpid()));struct PrepCleanup{std::filesystem::path p;~PrepCleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} prep_cleanup{prep_root};setenv("FSTR_LUT_PREPARATION_CACHE",prep_root.c_str(),1);try{
    check(argc>=2&&argc<=4,"Provide the actual built plugin executable");
    auto module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);cleanup_module=module;check(module!=nullptr,"Native plugin dlopen failed");
    effect_fn=reinterpret_cast<PF_FilterProc>(dlsym(module,"EffectMain"));check(effect_fn!=nullptr,"EffectMain export missing");
    if(argc==3&&(std::string(argv[2])=="--background-loading"||std::string(argv[2])=="--background-failure")){
        const bool failure=std::string(argv[2])=="--background-failure";
        if(failure){unsetenv("HOME");unsetenv("FSTR_LUT_AUTOMATIC_CACHE");}
        Fixture f;active=&f;const auto began=std::chrono::steady_clock::now();
        check(effect_fn(PF_Cmd_GLOBAL_SETUP,&f.in,&f.out,nullptr,nullptr,nullptr)==0,"Async global setup failed ordinary route");
        auto elapsed=[&]{return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count();};
        const auto setup_ms=elapsed();check(setup_ms<5000,"Setup waited on model factory");
        if(!failure){check(effect_fn(PF_Cmd_ABOUT,&f.in,&f.out,nullptr,nullptr,nullptr)==0&&std::strstr(f.out.return_msg,"loading model"),"Loading status absent");}
        std::vector<PF_PixelFloat> input{{1,.1f,.2f,.3f}},output(1);set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB128);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB128);
        f.params[Exposure].u.fs_d.value=1;f.params[SmartColor].u.bd.value=FALSE;
        check(run(f,true)==0&&output[0].red==.2f,"Off waited or failed during loading/failure");const auto off_ms=elapsed()-setup_ms;
        f.params[SmartColor].u.bd.value=TRUE;f.params[Look].u.pd.value=9;f.params[LookAmount].u.fs_d.value=65;
        if(!failure)f.abort_now=true;
        const auto before_cancel=elapsed();auto err=run(f,true);const auto cancel_ms=elapsed()-before_cancel;
        check(failure?err==PF_Err_BAD_CALLBACK_PARAM:err==75,"Active loading ignored cancellation/failure");check(cancel_ms<1000,"Cancellation waited for constructor");f.abort_now=false;
        f.params[SmartColor].u.bd.value=FALSE;check(run(f,true)==0,"Failed/cancelled wait broke ordinary grading");
        const auto before_stop=elapsed();check(effect_fn(PF_Cmd_GLOBAL_SETDOWN,&f.in,&f.out,nullptr,nullptr,nullptr)==0,"Async setdown failed");const auto stop_ms=elapsed()-before_stop;
        check(dlclose(module)==0,"Async unload failed");cleanup_module=nullptr;
        std::cout<<"PASS SDK callback background "<<(failure?"failure":"loading/cancel/setdown")<<" setup_ms="<<setup_ms<<" off_ms="<<off_ms<<" cancel_ms="<<cancel_ms<<" stop_ms="<<stop_ms<<"; actual AE cancellation NOT RUN\n";return 0;
    }
    Fixture setup;active=&setup;check(effect_fn(PF_Cmd_GLOBAL_SETUP,&setup.in,&setup.out,nullptr,nullptr,nullptr)==PF_Err_NONE,"Global setup failed");
    host_rows(PF_PixelFormat_ARGB32,PF_Pixel{255,80,60,40});host_rows(PF_PixelFormat_ARGB64,PF_Pixel16{32768,10000,7000,4000});host_rows(PF_PixelFormat_ARGB128,PF_PixelFloat{1,.2f,.1f,.05f});

    check(setup.out.out_flags==0x06008042&&setup.out.out_flags2==0x08221401&&setup.out.my_version==32769,"PiPL/runtime flags mismatch");
    check_dynamic_flags();
    Fixture ui;active=&ui;check(effect_fn(PF_Cmd_PARAMS_SETUP,&ui.in,&ui.out,nullptr,nullptr,nullptr)==PF_Err_NONE&&ui.out.num_params==ParamCount,"Native control schema failed");
    check(ui.params[Contrast].uu.id==3&&ui.params[WorkingSpace].uu.id==9&&ui.params[Temperature].uu.id==10&&ui.params[Tint].uu.id==11,"Persistent IDs changed with UI order");
    Fixture non_ae;non_ae.in.appl_id=0;check(effect_fn(PF_Cmd_GLOBAL_SETUP,&non_ae.in,&non_ae.out,nullptr,nullptr,nullptr)==PF_Err_INVALID_CALLBACK,"Non-AE host accepted");
    formats(PF_PixelFormat_ARGB32,PF_Pixel{127,51,75,99});
    formats(PF_PixelFormat_ARGB64,PF_Pixel16{12345,65535,7777,5123});
    formats(PF_PixelFormat_ARGB128,PF_PixelFloat{.5f,-.2f,2.0f,.1f});processing();cinema();failures();smart_requests();automatic_preparation();scene_timeline_callbacks();custom_lut_reference();if(argc==4&&std::string(argv[2])=="--automatic"){std::ifstream jobs(argv[3]);auto batch=nlohmann::json::parse(jobs);if(batch.contains("jobs")){for(const auto&path:batch.at("jobs"))automatic_scene(path.get<std::string>().c_str());}else automatic_scene(argv[3]);}else {if(argc>=3)real_smart(argv[2]);if(argc==4){real_recovery(argv[3]);for(int frame:{54,55,56})real_recovery(argv[3],frame,false);}}
    std::vector<std::future<bool>> tasks;
    for(int i=0;i<8;++i)tasks.push_back(std::async(std::launch::async,[]{Fixture f;std::vector<PF_PixelFloat> input{{1,.1f,.2f,.3f}},output(1);set_world(f.input,input,1,1,1,PF_PixelFormat_ARGB128);set_world(f.output,output,1,1,1,PF_PixelFormat_ARGB128);f.params[Exposure].u.fs_d.value=1;return run(f,true)==PF_Err_NONE&&output[0].red==.2f&&f.checkin==RenderParamCount&&f.pixels_in==1;}));
    for(auto& t:tasks)check(t.get(),"Parallel native snapshot mismatch");
    check(effect_fn(PF_Cmd_GLOBAL_SETDOWN,&setup.in,&setup.out,nullptr,nullptr,nullptr)==PF_Err_NONE,"Global setdown failed");
    cached_scene_bridge();
    smart_host_rows();
    check(dlclose(module)==0,"Native module cleanup failed");cleanup_module=nullptr;
    std::cout<<"PASS "<<checks<<" owned native callback assertions; actual AE loader/MFR: NOT RUN\n";return 0;
}catch(const std::exception& e){if(cleanup_module&&effect_fn){Fixture cleanup;active=&cleanup;effect_fn(PF_Cmd_GLOBAL_SETDOWN,&cleanup.in,&cleanup.out,nullptr,nullptr,nullptr);dlclose(cleanup_module);cleanup_module=nullptr;}std::cerr<<"FAIL "<<e.what()<<" assertion "<<checks<<"\n";return 1;}}
