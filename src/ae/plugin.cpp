#include "plugin.hpp"
#include "fstr_embedded_look.hpp"
#include "metadata_ui.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>
#include <type_traits>

namespace {
constexpr PF_OutFlags flags=PF_OutFlag_DEEP_COLOR_AWARE|PF_OutFlag_PIX_INDEPENDENT|PF_OutFlag_USE_OUTPUT_EXTENT|PF_OutFlag_CUSTOM_UI;
constexpr PF_OutFlags2 flags2=PF_OutFlag2_SUPPORTS_SMART_RENDER|PF_OutFlag2_FLOAT_COLOR_AWARE|PF_OutFlag2_SUPPORTS_THREADED_RENDERING;
static_assert(flags==0x02008440&&flags2==0x08001400,"PiPL flags must match runtime");
static_assert(PF_VERSION(0,1,0,PF_Stage_DEVELOP,1)==32769,"PiPL version must match runtime");
std::shared_ptr<const fstr::LUT> warm;
std::mutex initialization;
void initialize_look() {
    std::lock_guard<std::mutex> lock(initialization);
    if(!std::atomic_load(&warm)) {
        std::istringstream input(fstr::embedded_warm_v1);
        std::atomic_store(&warm,std::shared_ptr<const fstr::LUT>(std::make_shared<fstr::LUT>(fstr::LUT::parse(input))));
    }
}
class Params final {
    PF_InData* in_;
    std::array<PF_ParamDef,ParamCount> values_{};
    int acquired_=0;
public:
    explicit Params(PF_InData* in):in_(in) {
        for(int id=1;id<CameraInfo;++id) {
            auto err=PF_CHECKOUT_PARAM(in_,id,in_->current_time,in_->time_step,in_->time_scale,&values_[id]);
            if(err){close();throw err;}acquired_=id;
        }
    }
    ~Params(){close();}
    const PF_ParamDef& operator[](int id)const{return values_[id];}
    PF_Err close() noexcept {
        PF_Err first=PF_Err_NONE;
        while(acquired_>0){auto err=PF_CHECKIN_PARAM(in_,&values_[acquired_--]);if(!first)first=err;}
        return first;
    }
};
fstr::ColorSettings settings(const Params& p) {
    fstr::ColorSettings s;
    const auto source=p[Source].u.pd.value,look=p[Look].u.pd.value;
    if(source<1||source>2||look<1||look>2)throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
    s.source=source==2?fstr::SourceProfile::SonySLog3Cine:fstr::SourceProfile::Host;
    s.grade={p[Exposure].u.fs_d.value,p[Contrast].u.fs_d.value,p[Saturation].u.fs_d.value/100};
    s.grade.temperature=p[Temperature].u.fs_d.value/100;s.grade.tint=p[Tint].u.fs_d.value/100;
    s.shoulder=p[Shoulder].u.fs_d.value/100;s.look_amount=p[LookAmount].u.fs_d.value/100;s.mix=p[Mix].u.fs_d.value/100;
    if(look==2){s.look=std::atomic_load(&warm);if(!s.look)throw static_cast<PF_Err>(PF_Err_INTERNAL_STRUCT_DAMAGED);}
    return s;
}
fstr::Transfer host_transfer(const Params& p) {
    switch(p[WorkingSpace].u.pd.value){
        case 1:return static_cast<fstr::Transfer>(-1); // Unspecified: neutral/bypass safe, active grade rejects.
        case 2:return fstr::Transfer::SRGB;
        case 3:return fstr::Transfer::Linear;
        case 4:return fstr::Transfer::Gamma24;
    }
    throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
}
PF_Err params_setup(PF_InData* in_data,PF_OutData* out_data) {
    PF_Err err=PF_Err_NONE;
    PF_ParamDef def{};
    PF_ADD_POPUP("Input Profile",2,1,"Already converted / SDR|Sony S-Log3 / S-Gamut3.Cine",SourceId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Exposure",-10,10,-5,5,0,PF_Precision_HUNDREDTHS,0,0,ExposureId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Temperature",-100,100,-100,100,0,PF_Precision_HUNDREDTHS,0,0,TemperatureId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Tint",-100,100,-100,100,0,PF_Precision_HUNDREDTHS,0,0,TintId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Contrast",.5,1.5,.5,1.5,1,PF_Precision_HUNDREDTHS,0,0,ContrastId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Saturation",0,200,0,200,100,PF_Precision_HUNDREDTHS,PF_ValueDisplayFlag_PERCENT,0,SaturationId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Highlight Rolloff",0,100,0,100,0,PF_Precision_HUNDREDTHS,PF_ValueDisplayFlag_PERCENT,0,ShoulderId);
    AEFX_CLR_STRUCT(def);PF_ADD_POPUP("Look",2,1,"Neutral|Warm Film",LookId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Look Amount",0,100,0,100,60,PF_Precision_HUNDREDTHS,PF_ValueDisplayFlag_PERCENT,0,LookAmountId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Mix",0,100,0,100,100,PF_Precision_HUNDREDTHS,PF_ValueDisplayFlag_PERCENT,0,MixId);
    AEFX_CLR_STRUCT(def);PF_ADD_POPUP("AE Pixel Encoding",4,1,"Choose pixel encoding...|sRGB|Linear Rec.709 / sRGB|Rec.709 Gamma 2.4",WorkingSpaceId);
    AEFX_CLR_STRUCT(def);def.ui_flags=PF_PUI_CONTROL|PF_PUI_DONT_ERASE_CONTROL;def.ui_width=300;def.ui_height=64;
    PF_ADD_NULL("Camera metadata",CameraInfoId);
    PF_CustomUIInfo custom{};custom.events=PF_CustomEFlag_EFFECT;
    if(!err)err=in_data->inter.register_ui(in_data->effect_ref,&custom);
    out_data->num_params=ParamCount;return err;
}
template<class P>constexpr double channel_max() {
    if constexpr(std::is_same_v<P,PF_Pixel>)return PF_MAX_CHAN8;
    else if constexpr(std::is_same_v<P,PF_Pixel16>)return PF_MAX_CHAN16;
    else return 1.0;
}
template<class P>PF_Err process_world(PF_InData* in_data,const PF_EffectWorld& in,PF_EffectWorld& out,
                                    const fstr::ColorProcessor& processor,bool smart) {
    if(out.width<0||out.height<0||in.width<0||in.height<0)return PF_Err_BAD_CALLBACK_PARAM;
    if(out.width==0||out.height==0)return PF_Err_NONE;
    if(!out.data||out.rowbytes<=0||out.width>out.rowbytes/static_cast<A_long>(sizeof(P)))return PF_Err_BAD_CALLBACK_PARAM;
    if(in.width&&in.height&&(!in.data||in.rowbytes<=0||in.width>in.rowbytes/static_cast<A_long>(sizeof(P))))return PF_Err_BAD_CALLBACK_PARAM;
    const auto scale=channel_max<P>();
    // SmartFX origins are layer coordinates. Legacy does not change buffer size.
    const std::int64_t dx=smart?std::int64_t(out.origin_x)-in.origin_x:-std::int64_t(in_data->output_origin_x);
    const std::int64_t dy=smart?std::int64_t(out.origin_y)-in.origin_y:-std::int64_t(in_data->output_origin_y);
    for(A_long y=0;y<out.height;++y) {
        const auto abort=PF_ABORT(in_data);if(abort)return abort;
        auto dst=reinterpret_cast<P*>(reinterpret_cast<unsigned char*>(out.data)+std::size_t(y)*std::size_t(out.rowbytes));
        const auto sy=std::int64_t(y)+dy;
        for(A_long x=0;x<out.width;++x) {
            const auto sx=std::int64_t(x)+dx;
            if(sx<0||sy<0||sx>=in.width||sy>=in.height){dst[x]={};continue;}
            const auto src=reinterpret_cast<const P*>(reinterpret_cast<const unsigned char*>(in.data)+std::size_t(sy)*std::size_t(in.rowbytes))+sx;
            if(processor.bypass()||src->alpha==0){dst[x]=*src;continue;}
            const auto v=processor.apply({float(src->red/scale),float(src->green/scale),float(src->blue/scale),float(src->alpha/scale)});
            if constexpr(std::is_same_v<P,PF_PixelFloat>){dst[x]={src->alpha,v[0],v[1],v[2]};}
            else {
                auto quantize=[scale](float value){return static_cast<decltype(P::red)>(std::clamp(double(value),0.0,1.0)*scale+.5);};
                dst[x]={src->alpha,quantize(v[0]),quantize(v[1]),quantize(v[2])};
            }
        }
    }
    return PF_Err_NONE;
}
PF_Err render(PF_InData* in_data,PF_EffectWorld& input,PF_EffectWorld& output,bool smart) {
    if(output.width==0||output.height==0)return PF_Err_NONE;
    Params params(in_data);const auto s=settings(params);const auto transfer=host_transfer(params);
    const auto primaries=static_cast<int>(transfer)<0?fstr::Primaries::Unknown:fstr::Primaries::SRGB_Rec709;
    const fstr::ColorProcessor processor(s,transfer,primaries,fstr::Alpha::Premultiplied);
    const PF_WorldSuite2* world_suite=nullptr;
    auto err=in_data->pica_basicP->AcquireSuite(kPFWorldSuite,kPFWorldSuiteVersion2,reinterpret_cast<const void**>(&world_suite));
    if(err)throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
    PF_PixelFormat format{},out_format{};
    err=world_suite->PF_GetPixelFormat(&output,&out_format);
    if(!err) {
        if(input.width==0||input.height==0)format=out_format;
        else err=world_suite->PF_GetPixelFormat(&input,&format);
    }
    try {
        if(!err&&format!=out_format)err=PF_Err_BAD_CALLBACK_PARAM;
        if(!err)switch(format) {
            case PF_PixelFormat_ARGB32:err=process_world<PF_Pixel>(in_data,input,output,processor,smart);break;
            case PF_PixelFormat_ARGB64:err=process_world<PF_Pixel16>(in_data,input,output,processor,smart);break;
            case PF_PixelFormat_ARGB128:err=process_world<PF_PixelFloat>(in_data,input,output,processor,smart);break;
            default:err=PF_Err_BAD_CALLBACK_PARAM;
        }
    }catch(...){in_data->pica_basicP->ReleaseSuite(kPFWorldSuite,kPFWorldSuiteVersion2);throw;}
    const auto release=in_data->pica_basicP->ReleaseSuite(kPFWorldSuite,kPFWorldSuiteVersion2);
    const auto checked=params.close();return err?err:(release?PF_Err_BAD_CALLBACK_PARAM:checked);
}
PF_Err pre_render(PF_InData* in_data,PF_PreRenderExtra* extra) {
    auto request=extra->input->output_request;request.channel_mask=PF_ChannelMask_ARGB;request.preserve_rgb_of_zero_alpha=TRUE;
    PF_CheckoutResult result{};
    const auto err=extra->cb->checkout_layer(in_data->effect_ref,Input,Input,&request,in_data->current_time,in_data->time_step,in_data->time_scale,&result);
    if(!err){extra->output->result_rect=result.result_rect;extra->output->max_result_rect=result.max_result_rect;}
    return err;
}
PF_Err smart_render(PF_InData* in_data,PF_SmartRenderExtra* extra) {
    PF_EffectWorld *input=nullptr,*output=nullptr;
    auto err=extra->cb->checkout_layer_pixels(in_data->effect_ref,Input,&input);if(err)return err;
    try {
        err=extra->cb->checkout_output(in_data->effect_ref,&output);
        if(!err)err=(!input||!output)?PF_Err_BAD_CALLBACK_PARAM:render(in_data,*input,*output,true);
    }catch(...){extra->cb->checkin_layer_pixels(in_data->effect_ref,Input);throw;}
    const auto release=extra->cb->checkin_layer_pixels(in_data->effect_ref,Input);return err?err:release;
}
}
extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr data,PF_PluginDataCB2 callback,SPBasicSuite*,const char*,const char*) {
    PF_Err result=PF_Err_INVALID_CALLBACK;
    result=PF_REGISTER_EFFECT_EXT2(data,callback,"FSTR-LUT Development","FSTR LUT Color MVP","FSTR.Tools",AE_RESERVED_INFO,"EffectMain","https://github.com/ios3kov/FSTR_LUT");
    return result;
}
extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd,PF_InData* in_data,PF_OutData* out_data,PF_ParamDef* params[],PF_LayerDef* output,void* extra) {
    try {
        switch(cmd) {
            case PF_Cmd_ABOUT:std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"FSTR-LUT color MVP 0.1 development\rBuild %s\rSDR + Sony S-Log3 / S-Gamut3.Cine",fstr::color_build_id);break;
            case PF_Cmd_GLOBAL_SETUP:if(in_data->appl_id!=kAppID_AfterEffects)return PF_Err_INVALID_CALLBACK;initialize_look();out_data->my_version=32769;out_data->out_flags=flags;out_data->out_flags2=flags2;break;
            case PF_Cmd_GLOBAL_SETDOWN:std::atomic_store(&warm,std::shared_ptr<const fstr::LUT>{});fstr::ae::clear_all_metadata_cache();break;
            case PF_Cmd_SEQUENCE_SETUP:case PF_Cmd_SEQUENCE_SETDOWN:fstr::ae::clear_metadata_cache(in_data->effect_ref);break;
            case PF_Cmd_PARAMS_SETUP:return params_setup(in_data,out_data);
            case PF_Cmd_EVENT:return fstr::ae::metadata_event(in_data,static_cast<PF_EventExtra*>(extra));
            case PF_Cmd_RENDER:return render(in_data,params[Input]->u.ld,*output,false);
            case PF_Cmd_SMART_PRE_RENDER:return pre_render(in_data,static_cast<PF_PreRenderExtra*>(extra));
            case PF_Cmd_SMART_RENDER:return smart_render(in_data,static_cast<PF_SmartRenderExtra*>(extra));
            default:break;
        }
        return PF_Err_NONE;
    }catch(PF_Err err){return err;}
     catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}
     catch(const fstr::UnsupportedColorProfile&){std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"Choose AE Pixel Encoding. Linearized project: select Linear. Sony override requires preserved, unconverted Log samples.");return PF_Err_BAD_CALLBACK_PARAM;}
     catch(...){std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"FSTR-LUT color processing failed; input, settings or range unsupported.");return PF_Err_BAD_CALLBACK_PARAM;}
}
