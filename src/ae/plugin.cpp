#include "plugin.hpp"
#include "fstr_embedded_look.hpp"
#include "metadata_ui.hpp"
#include "fstr/regions.hpp"
#include "fstr/scene_cache.hpp"
#include "fstr/automatic_scene_cache.hpp"
#include "fstr/scene_window.hpp"
#include "fstr/sdr_prepare_cache.hpp"
#include <optional>
#include "picosha2.h"
#include <iomanip>
#include <locale>
#ifdef FSTR_NATIVE_SMART
#include "fstr/native_scene.hpp"
#include <dlfcn.h>
#include <filesystem>
#endif
#include <cstdlib>
#include <limits>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <sstream>
#include <type_traits>

namespace {
constexpr PF_OutFlags flags=PF_OutFlag_DEEP_COLOR_AWARE|PF_OutFlag_WIDE_TIME_INPUT|PF_OutFlag_USE_OUTPUT_EXTENT|PF_OutFlag_CUSTOM_UI;
constexpr PF_OutFlags2 flags2=PF_OutFlag2_SUPPORTS_SMART_RENDER|PF_OutFlag2_FLOAT_COLOR_AWARE|PF_OutFlag2_SUPPORTS_THREADED_RENDERING|PF_OutFlag2_AUTOMATIC_WIDE_TIME_INPUT|PF_OutFlag2_I_MIX_GUID_DEPENDENCIES|PF_OutFlag2_SUPPORTS_QUERY_DYNAMIC_FLAGS;
static_assert(flags==0x02008042&&flags2==0x08221401,"PiPL flags must match runtime");
static_assert(PF_VERSION(0,1,0,PF_Stage_DEVELOP,1)==32769,"PiPL version must match runtime");
using LookLibrary=std::array<std::shared_ptr<const fstr::LUT>,fstr::builtin_look_count>;
std::shared_ptr<const LookLibrary> looks;
std::mutex initialization;
std::shared_ptr<fstr::SceneCache> scene_cache;
std::shared_ptr<fstr::AutomaticSceneCache> automatic_cache;
std::shared_ptr<fstr::SdrPreparationCache> preparation_cache;
std::shared_ptr<const std::string> automatic_error;
class SmartAnalysisError final : public std::runtime_error {public:using std::runtime_error::runtime_error;};
class PreparationError final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
void initialize_preparation_cache(){
    std::lock_guard<std::mutex> lock(initialization);
    if(std::atomic_load(&preparation_cache))return;
    std::string path;if(const char* configured=std::getenv("FSTR_LUT_PREPARATION_CACHE"))path=configured;
    else if(const char* home=std::getenv("HOME"))path=std::string(home)+"/Library/Caches/FSTR.Tools/FSTR_LUT/sdr-preparation-v2";
    try{std::atomic_store(&preparation_cache,std::make_shared<fstr::SdrPreparationCache>(path));}
    catch(const std::exception&){std::atomic_store(&preparation_cache,std::make_shared<fstr::SdrPreparationCache>());}
}
// Preserve the exact old path when preparation is absent/neutral. Mix includes
// preparation as well as manual grading/look; alpha and hidden RGB are unchanged.
class ColorPass final {
    fstr::ColorProcessor legacy_;
    std::unique_ptr<fstr::ColorProcessor> full_;
    fstr::SdrPreparation preparation_;
    fstr::Transfer transfer_;double mix_;
public:
    ColorPass(fstr::ColorSettings s,fstr::Transfer transfer,fstr::Primaries primaries,const fstr::SdrPreparation* preparation)
        :legacy_(s,transfer,primaries,fstr::Alpha::Premultiplied),transfer_(transfer),mix_(s.mix){
        if(preparation&&s.mix>0&&(preparation->contrast!=1||preparation->brightness_bias!=0||preparation->saturation!=1)){
            preparation_=*preparation;s.mix=1;full_=std::make_unique<fstr::ColorProcessor>(s,transfer,primaries,fstr::Alpha::Premultiplied);
        }
    }
    bool bypass()const noexcept{return !full_&&legacy_.bypass();}
    std::array<float,4> apply(std::array<float,4> raw,double protection=0)const{
        if(!full_||raw[3]==0)return legacy_.apply(raw,protection);
        const double alpha=raw[3];if(!std::isfinite(alpha)||alpha<=0||alpha>1)throw PreparationError("Invalid SDR alpha");
        const fstr::RGB base{fstr::decode_host(raw[0]/alpha,transfer_),fstr::decode_host(raw[1]/alpha,transfer_),fstr::decode_host(raw[2]/alpha,transfer_)};
        fstr::RGB value;try{value=fstr::prepare_sdr_linear(base,preparation_);}catch(const std::invalid_argument&){throw PreparationError("Auto Prepare requires bounded SDR pixels");}
        auto result=full_->apply({float(fstr::encode_host(value.r,transfer_)*alpha),float(fstr::encode_host(value.g,transfer_)*alpha),float(fstr::encode_host(value.b,transfer_)*alpha),raw[3]},protection);
        if(mix_<1)for(std::size_t c=0;c<3;++c){const double source=fstr::decode_host(raw[c]/alpha,transfer_),styled=fstr::decode_host(result[c]/alpha,transfer_);result[c]=float(fstr::encode_host(source+mix_*(styled-source),transfer_)*alpha);}
        return result;
    }
};
void initialize_automatic_analysis(){
#ifdef FSTR_NATIVE_SMART
    std::lock_guard<std::mutex> lock(initialization);
    if(std::atomic_load(&automatic_cache)||std::atomic_load(&automatic_error))return;
    try{
        if(std::atomic_load(&scene_cache))return; // Explicit prepared development snapshot retains its original path.
        std::filesystem::path resources;
        if(const char* configured=std::getenv("FSTR_LUT_NATIVE_RESOURCES"))resources=configured;
        else {Dl_info info{};if(!dladdr(reinterpret_cast<void*>(&EffectMain),&info)||!info.dli_fname)throw std::runtime_error("Smart resources unavailable");resources=std::filesystem::path(info.dli_fname).parent_path().parent_path()/"Resources"/"Smart";}
        auto analyzer=std::make_shared<fstr::NativeSceneAnalyzer>(resources.string());
        std::filesystem::path directory;
        if(const char* configured=std::getenv("FSTR_LUT_AUTOMATIC_CACHE"))directory=configured;
        else {const char* user_home=std::getenv("HOME");if(!user_home)throw std::runtime_error("Smart cache location unavailable");directory=std::filesystem::path(user_home)/"Library/Caches/FSTR.Tools/FSTR_LUT/automatic-v1";}
        auto cache=std::make_shared<fstr::AutomaticSceneCache>(directory.string(),analyzer->policy(),[analyzer](const auto&image,double height,const auto&cancel){return analyzer->analyze(image,height,cancel);});
        std::atomic_store(&automatic_cache,std::move(cache));
    }catch(const std::exception& e){std::atomic_store(&automatic_error,std::make_shared<const std::string>(std::string("Smart analysis unavailable: ")+e.what()));}
#endif
}
void initialize_scene_cache(){
    const char* directory=std::getenv("FSTR_LUT_SCENE_CACHE");const char* policy=std::getenv("FSTR_LUT_SCENE_POLICY");
    if(!directory&&!policy)return;
    if(!directory||!policy)throw std::invalid_argument("Incomplete scene cache configuration");
    std::atomic_store(&scene_cache,std::make_shared<fstr::SceneCache>(directory,policy));
}
void initialize_look() {
    std::lock_guard<std::mutex> lock(initialization);
    if(!std::atomic_load(&looks)) {
        auto library=std::make_shared<LookLibrary>();
        for(std::size_t i=0;i<library->size();++i)if(fstr::embedded_looks[i].cube) {
            std::istringstream input(fstr::embedded_looks[i].cube);
            (*library)[i]=std::make_shared<const fstr::LUT>(fstr::LUT::parse(input));
        }
        // Publish the complete immutable library only after every parse succeeds.
        std::atomic_store(&looks,std::shared_ptr<const LookLibrary>(std::move(library)));
    }
}
class Params final {
    PF_InData* in_;
    std::array<PF_ParamDef,ParamCount> values_{};
    std::array<int,RenderParamCount> acquired_{};
    int count_=0;
public:
    explicit Params(PF_InData* in):in_(in) {
        for(int id=1;id<ParamCount;++id) {
            if(id==AdvancedStart||id==AdvancedEnd||id==CameraInfo)continue;
            auto err=PF_CHECKOUT_PARAM(in_,id,in_->current_time,in_->time_step,in_->time_scale,&values_[id]);
            if(err){close();throw err;}acquired_[count_++]=id;
        }
    }
    ~Params(){close();}
    const PF_ParamDef& operator[](int id)const{return values_[id];}
    PF_Err close() noexcept {
        PF_Err first=PF_Err_NONE;
        while(count_>0){auto err=PF_CHECKIN_PARAM(in_,&values_[acquired_[--count_]]);if(!first)first=err;}
        return first;
    }
};
fstr::ColorSettings settings(const Params& p) {
    fstr::ColorSettings s;
    const auto source=p[Source].u.pd.value,look=p[Look].u.pd.value;
    if(source<1||source>fstr::source_profile_count||look<1||look>static_cast<int>(fstr::builtin_look_count))throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
    s.source=static_cast<fstr::SourceProfile>(source-1);
    s.grade={p[Exposure].u.fs_d.value,p[Contrast].u.fs_d.value,p[Saturation].u.fs_d.value/100};
    s.grade.natural_contrast=p[NaturalContrast].u.bd.value!=0;
    s.grade.temperature=p[Temperature].u.fs_d.value/100;s.grade.tint=p[Tint].u.fs_d.value/100;
    s.shoulder=p[Shoulder].u.fs_d.value/100;s.look_amount=p[LookAmount].u.fs_d.value/100;s.mix=p[Mix].u.fs_d.value/100;
    if(look>1) {
        const auto library=std::atomic_load(&looks);
        if(!library)throw static_cast<PF_Err>(PF_Err_INTERNAL_STRUCT_DAMAGED);
        s.look=(*library)[static_cast<std::size_t>(look-1)];
        if(!s.look)throw static_cast<PF_Err>(PF_Err_INTERNAL_STRUCT_DAMAGED);
    }
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
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Exposure",-10,10,-5,5,0,PF_Precision_HUNDREDTHS,0,0,ExposureId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Temperature",-100,100,-100,100,0,PF_Precision_HUNDREDTHS,0,0,TemperatureId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Tint",-100,100,-100,100,0,PF_Precision_HUNDREDTHS,0,0,TintId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Contrast",.5,1.5,.5,1.5,1,PF_Precision_HUNDREDTHS,0,0,ContrastId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Saturation",0,200,0,200,100,PF_Precision_HUNDREDTHS,PF_ValueDisplayFlag_PERCENT,0,SaturationId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Highlight Rolloff",0,100,0,100,0,PF_Precision_HUNDREDTHS,PF_ValueDisplayFlag_PERCENT,0,ShoulderId);
    // Slot2 is a nonselectable separator; old projects keep their frozen Warm v1.
    AEFX_CLR_STRUCT(def);PF_ADD_POPUP("Look",static_cast<A_short>(fstr::builtin_look_count),1,fstr::builtin_look_menu,LookId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Look Amount",0,300,0,100,100,PF_Precision_HUNDREDTHS,PF_ValueDisplayFlag_PERCENT,0,LookAmountId);
    AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Mix",0,100,0,100,100,PF_Precision_HUNDREDTHS,PF_ValueDisplayFlag_PERCENT,0,MixId);
    PF_ADD_TOPICX("Advanced",PF_ParamFlag_COLLAPSE_TWIRLY,AdvancedStartId);
    AEFX_CLR_STRUCT(def);PF_ADD_POPUP("Input Profile",static_cast<A_short>(fstr::source_profile_count),1,fstr::source_profile_menu,SourceId);
    AEFX_CLR_STRUCT(def);PF_ADD_POPUP("AE Pixel Encoding",4,2,"Choose pixel encoding...|sRGB|Linear Rec.709 / sRGB|Rec.709 Gamma 2.4",WorkingSpaceId);
    // SDK old-project value0 retains legacy math; freshly applied/reset effects use1.
    AEFX_CLR_STRUCT(def);def.param_type=PF_Param_CHECKBOX;
    PF_STRNNCPY(def.PF_DEF_NAME,"Natural Contrast",sizeof(def.PF_DEF_NAME));
    def.u.bd.u.PF_DEF_NAMEPTR="On";def.u.bd.value=FALSE;def.u.bd.dephault=TRUE;
    def.flags=PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS;def.uu.id=NaturalContrastId;
    if((err=PF_ADD_PARAM(in_data,-1,&def))!=PF_Err_NONE)return err;
    AEFX_CLR_STRUCT(def);PF_END_TOPIC(AdvancedEndId);
    AEFX_CLR_STRUCT(def);PF_ADD_CHECKBOX("Smart Color Grading","On",FALSE,0,SmartColorId);
    AEFX_CLR_STRUCT(def);def.ui_flags=PF_PUI_CONTROL|PF_PUI_DONT_ERASE_CONTROL;def.ui_width=300;def.ui_height=64;
    PF_ADD_NULL("Camera metadata",CameraInfoId);
    PF_CustomUIInfo custom{};custom.events=PF_CustomEFlag_EFFECT;
    if(!err)err=in_data->inter.register_ui(in_data->effect_ref,&custom);
    AEFX_CLR_STRUCT(def);PF_ADD_CHECKBOX("Auto Prepare SDR","On",FALSE,0,AutoPrepareId);
    out_data->num_params=ParamCount;return err;
}
template<class P>constexpr double channel_max() {
    if constexpr(std::is_same_v<P,PF_Pixel>)return PF_MAX_CHAN8;
    else if constexpr(std::is_same_v<P,PF_Pixel16>)return PF_MAX_CHAN16;
    else return 1.0;
}
struct TemporalInput {PF_EffectWorld* world=nullptr;int offset=0;};
struct PreparationInput {PF_EffectWorld* world=nullptr;A_long time=0;};
struct SmartPreData {std::vector<std::pair<int,int>> frames;std::vector<std::pair<int,A_long>> anchors;};
void delete_smart_pre(void* data){delete static_cast<SmartPreData*>(data);}
template<class P>std::string analysis_source_identity(PF_InData*in_data,const PF_EffectWorld&world,const fstr::ColorSettings&s,fstr::Transfer transfer,const std::string& preparation_key={}){
    if(world.width<1||world.height<1||!world.data||world.rowbytes<=0||world.width>world.rowbytes/static_cast<A_long>(sizeof(P)))throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
    std::ostringstream meta;meta.imbue(std::locale::classic());meta<<std::setprecision(17)<<"FSTR_UPSTREAM_V1 "<<sizeof(P)<<' '<<world.width<<' '<<world.height<<' '<<world.origin_x<<' '<<world.origin_y<<' '<<int(s.source)<<' '<<int(transfer)<<' '<<s.grade.exposure_stops<<' '<<s.grade.contrast<<' '<<s.grade.saturation<<' '<<s.grade.temperature<<' '<<s.grade.tint<<' '<<s.shoulder;
    if(s.grade.natural_contrast&&s.grade.contrast!=1)meta<<" NATURAL_CONTRAST_V1";
    if(!preparation_key.empty())meta<<" SDR_PREP_V2 "<<preparation_key;
    picosha2::hash256_one_by_one hash;hash.init();auto text=meta.str();hash.process(text.begin(),text.end());
    // Hash valid channels only, excluding undefined row padding and ROI output.
    for(A_long y=0;y<world.height;++y){auto error=PF_ABORT(in_data);if(error)throw error;auto row=reinterpret_cast<const P*>(reinterpret_cast<const char*>(world.data)+std::size_t(y)*std::size_t(world.rowbytes));for(A_long x=0;x<world.width;++x){const auto&p=row[x];for(const auto*channel:{&p.alpha,&p.red,&p.green,&p.blue}){auto begin=reinterpret_cast<const unsigned char*>(channel);hash.process(begin,begin+sizeof(*channel));}}}hash.finish();return picosha2::get_hash_hex_string(hash);
}
template<class P,class Processor>fstr::AnalysisImage analysis_image(PF_InData* in_data,const PF_EffectWorld& world,const Processor& prepare,fstr::Transfer transfer){
    if(world.width<=0||world.height<=0||!world.data||world.rowbytes<=0||world.width>world.rowbytes/static_cast<A_long>(sizeof(P)))throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
    double aspect=1.;
    if(in_data->pixel_aspect_ratio.num>0&&in_data->pixel_aspect_ratio.den>0)aspect=double(in_data->pixel_aspect_ratio.num)/in_data->pixel_aspect_ratio.den;
    if(in_data->downsample_x.num>0&&in_data->downsample_x.den>0&&in_data->downsample_y.num>0&&in_data->downsample_y.den>0)
        aspect*=double(in_data->downsample_x.den)/in_data->downsample_x.num*in_data->downsample_y.num/in_data->downsample_y.den;
    if(!std::isfinite(aspect)||aspect<.01||aspect>100)throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
    const double factor=std::min(1.,(std::atomic_load(&automatic_cache)?384.:960.)/std::max(world.width*aspect,double(world.height)));fstr::AnalysisImage image;
    image.width=std::max(1,int(world.width*aspect*factor));image.height=std::max(1,int(world.height*factor));image.rgba.resize(std::size_t(image.width)*image.height*4);
    const double scale=channel_max<P>();
    for(int y=0;y<image.height;++y){auto abort=PF_ABORT(in_data);if(abort)throw abort;
        int sy=std::min<int>(world.height-1,(y+.5)*world.height/image.height);auto row=reinterpret_cast<const P*>(reinterpret_cast<const char*>(world.data)+std::size_t(sy)*world.rowbytes);
        for(int x=0;x<image.width;++x){auto p=row[std::min<int>(world.width-1,(x+.5)*world.width/image.width)];auto out=&image.rgba[(std::size_t(y)*image.width+x)*4];out[3]=255;
            if(p.alpha==0){out[0]=out[1]=out[2]=0;continue;}
            auto v=prepare.apply({float(p.red/scale),float(p.green/scale),float(p.blue/scale),float(p.alpha/scale)});
            for(int c=0;c<3;++c)out[c]=static_cast<std::uint8_t>(std::clamp(fstr::encode_host(fstr::decode_host(v[c]/v[3],transfer),fstr::Transfer::SRGB),0.,1.)*255+.5);
        }
    }return image;
}
bool compatible_image(const fstr::AnalysisImage&a,const fstr::AnalysisImage&b){
    if(a.width!=b.width||a.height!=b.height)return false;double sum=0;std::size_t count=0;
    for(std::size_t i=0;i<a.rgba.size();i+=64){for(int c=0;c<3;++c){sum+=std::abs(int(a.rgba[i+c])-int(b.rgba[i+c]));++count;}}
    return count&&sum/count<25.; // conservative whole-frame cut/appearance gate
}
template<class P>fstr::CachedSdrPreparation clip_preparation(PF_InData* in_data,const std::vector<PreparationInput>& anchors,fstr::Transfer transfer){
    const auto times=fstr::sdr_clip_anchor_times(in_data->total_time,in_data->time_step);
    if(anchors.size()!=times.size())throw PreparationError("Fixed clip anchors unavailable");
    const auto cache=std::atomic_load(&preparation_cache);if(!cache)throw PreparationError("Preparation cache unavailable");
    std::vector<std::vector<float>> snapshots;std::vector<fstr::FrameView> views;std::ostringstream context;context.imbue(std::locale::classic());
    context<<"AE_CLIP_V1 "<<in_data->total_time<<' '<<in_data->time_step<<' '<<in_data->time_scale;
    for(std::size_t i=0;i<anchors.size();++i){
        const auto* w=anchors[i].world;if(!w||anchors[i].time!=times[i]||!w->data||w->width<=0||w->height<=0||w->rowbytes<=0||w->width>w->rowbytes/static_cast<A_long>(sizeof(P)))throw PreparationError("Invalid clip anchor");
        if(i&&(w->width!=anchors[0].world->width||w->height!=anchors[0].world->height||w->origin_x!=anchors[0].world->origin_x||w->origin_y!=anchors[0].world->origin_y))throw PreparationError("Clip anchor geometry changed");
        context<<' '<<anchors[i].time<<' '<<w->width<<' '<<w->height<<' '<<w->origin_x<<' '<<w->origin_y<<' '<<sizeof(P);
        const std::size_t nx=std::min<A_long>(96,w->width),ny=std::min<A_long>(54,w->height);snapshots.emplace_back(nx*ny*4);auto& data=snapshots.back();const double scale=channel_max<P>();
        for(std::size_t y=0;y<ny;++y){auto error=PF_ABORT(in_data);if(error)throw error;const auto sy=(2*y+1)*std::size_t(w->height)/(2*ny);const auto row=reinterpret_cast<const P*>(reinterpret_cast<const char*>(w->data)+sy*std::size_t(w->rowbytes));
            for(std::size_t x=0;x<nx;++x){const auto& p=row[(2*x+1)*std::size_t(w->width)/(2*nx)];auto* out=data.data()+(y*nx+x)*4;out[0]=float(p.red/scale);out[1]=float(p.green/scale);out[2]=float(p.blue/scale);out[3]=float(p.alpha/scale);}
        }
    }
    // Build borrowed views only after snapshot storage has finished growing.
    for(const auto& data:snapshots){const auto w=std::min<A_long>(96,anchors[0].world->width),h=std::min<A_long>(54,anchors[0].world->height);views.push_back({data.data(),data.size(),std::size_t(w),std::size_t(h),std::size_t(w)*4,transfer,fstr::Primaries::SRGB_Rec709,fstr::Alpha::Premultiplied});}
    auto cancel=[&]{auto error=PF_ABORT(in_data);if(error)throw error;};
    try{return cache->get(views,context.str(),cancel);}catch(const std::invalid_argument&){throw PreparationError("Auto Prepare requires normal SDR input");}
}
template<class P>PF_Err process_world(PF_InData* in_data,const PF_EffectWorld& in,PF_EffectWorld& out,
                                    bool smart,bool enabled,fstr::Transfer transfer,const std::vector<TemporalInput>& neighbors,const fstr::ColorSettings& settings,bool scene_residual,bool auto_prepare,const std::vector<PreparationInput>& anchors) {
    if(out.width<0||out.height<0||in.width<0||in.height<0)return PF_Err_BAD_CALLBACK_PARAM;
    if(out.width==0||out.height==0)return PF_Err_NONE;
    if(!out.data||out.rowbytes<=0||out.width>out.rowbytes/static_cast<A_long>(sizeof(P)))return PF_Err_BAD_CALLBACK_PARAM;
    if(in.width&&in.height&&(!in.data||in.rowbytes<=0||in.width>in.rowbytes/static_cast<A_long>(sizeof(P))))return PF_Err_BAD_CALLBACK_PARAM;
    std::optional<fstr::CachedSdrPreparation> prepared;
    if(auto_prepare)prepared=clip_preparation<P>(in_data,anchors,transfer);
    const auto* recipe=prepared?&prepared->shot.settings:nullptr;const std::string preparation_key=prepared?prepared->key:std::string{};
    const auto primaries=static_cast<int>(transfer)<0?fstr::Primaries::Unknown:fstr::Primaries::SRGB_Rec709;
    const ColorPass processor(settings,transfer,primaries,recipe);
    auto preparation=settings;preparation.look.reset();preparation.look_amount=0;preparation.mix=1;
    const ColorPass prepare(enabled?preparation:settings,transfer,primaries,recipe);
    std::vector<fstr::ColorRegion> regions;
    const auto cache=std::atomic_load(&scene_cache);const auto automatic=std::atomic_load(&automatic_cache);const bool scene_active=bool(cache)||bool(automatic);std::shared_ptr<const fstr::SceneAnalysis> analysis;
    std::unique_ptr<fstr::SceneLook> scene;std::unique_ptr<fstr::ColorProcessor> scene_plain;
    std::vector<fstr::SkinToneReference> references;std::vector<fstr::SkinToneInfluence> influences;std::vector<fstr::StableToneWeight> stable;
    double source_height=in.height;
    if(in_data->downsample_y.num>0&&in_data->downsample_y.den>0)source_height*=double(in_data->downsample_y.den)/in_data->downsample_y.num;
    if(enabled&&!processor.bypass()&&in.width>0&&in.height>0){
        if(const auto error=std::atomic_load(&automatic_error))throw SmartAnalysisError(*error);
        if(scene_active){
            auto image=analysis_image<P>(in_data,in,prepare,transfer);
            if(automatic){
                auto cancel=[&]{auto error=PF_ABORT(in_data);if(error)throw error;};
                auto analyzed=[&](const auto&pixels,const std::string& identity){try{return automatic->get(pixels,source_height,cancel,identity);}catch(const std::bad_alloc&){throw;}catch(const std::exception&e){if(std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_SMART_ANALYSIS_ERROR %s\n",e.what());throw SmartAnalysisError("Smart analysis failed");}};
                std::vector<fstr::AnalyzedFrame> frames{{0,analyzed(image,analysis_source_identity<P>(in_data,in,settings,transfer,preparation_key))}};
                struct RawInput{int first;fstr::AnalysisImage second;std::string identity;};std::vector<RawInput> images;
                for(const auto&n:neighbors){if(!n.world||n.world->width!=in.width||n.world->height!=in.height||n.world->origin_x!=in.origin_x||n.world->origin_y!=in.origin_y)continue;auto other=analysis_image<P>(in_data,*n.world,prepare,transfer);if(compatible_image(image,other))images.push_back({n.offset,std::move(other),analysis_source_identity<P>(in_data,*n.world,settings,transfer,preparation_key)});}
                // No borrowing across an intervening missing/incompatible input.
                for(const auto&entry:images){if(std::abs(entry.first)==2){auto middle=std::find_if(images.begin(),images.end(),[&](const auto&v){return v.first==entry.first/2;});if(middle==images.end()||!compatible_image(middle->second,entry.second))continue;}frames.push_back({entry.first,analyzed(entry.second,entry.identity)});}
                analysis=fstr::stabilize_scene_window(frames);
            }else analysis=cache->get(image,source_height);
            if(analysis){scene=std::make_unique<fstr::SceneLook>(settings.look,settings.look_amount);fstr::ColorSettings plain;plain.look=settings.look;plain.look_amount=settings.look_amount;scene_plain=std::make_unique<fstr::ColorProcessor>(plain,fstr::Transfer::Linear,fstr::Primaries::SRGB_Rec709,fstr::Alpha::Straight);
                references.reserve(analysis->people.size());for(const auto&person:analysis->people){auto abort=PF_ABORT(in_data);if(abort)throw abort;references.push_back(scene->skin_reference(person.samples));}
                stable.resize(references.size());influences.resize(references.size());for(std::size_t i=0;i<references.size();++i)influences[i].reference=&references[i];
            }
        }else{
        std::vector<fstr::RegionFrame> window;
        auto current=analysis_image<P>(in_data,in,prepare,transfer);regions=fstr::detect_color_regions(current,source_height);
        window.push_back({0,std::move(current),regions});
        std::vector<fstr::ColorRegion> left,right;
        for(const auto&n:neighbors){if(!n.world||n.world->width!=in.width||n.world->height!=in.height||n.world->origin_x!=in.origin_x||n.world->origin_y!=in.origin_y)continue;
            auto image=analysis_image<P>(in_data,*n.world,prepare,transfer);
            auto detected=fstr::detect_color_regions(image,source_height);
            if(compatible_image(window.front().image,image)){if(n.offset==-1)left=detected;else if(n.offset==1)right=detected;}
            window.push_back({n.offset,std::move(image),std::move(detected)});
        }
        auto recovered=fstr::recover_color_regions(window);
        auto stable=fstr::stabilize_color_regions(regions,left,right);
        stable.insert(stable.end(),recovered.begin()+regions.size(),recovered.end());regions=std::move(stable);
        }
    }
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
            const std::array<float,4> raw{float(src->red/scale),float(src->green/scale),float(src->blue/scale),float(src->alpha/scale)};
            auto v=processor.apply(raw,enabled&&!scene_active?.8*fstr::color_region_weight((sx+.5)/in.width,(sy+.5)/in.height,source_height,regions):0);
            if(analysis){
                const double ax=std::clamp((sx+.5)*analysis->width/in.width-.5,0.,double(analysis->width-1)),ay=std::clamp((sy+.5)*analysis->height/in.height-.5,0.,double(analysis->height-1));
                const int x0=int(ax),y0=int(ay),x1=std::min(x0+1,analysis->width-1),y1=std::min(y0+1,analysis->height-1);const double fx=ax-x0,fy=ay-y0;
                const std::size_t indices[]{std::size_t(y0)*analysis->width+x0,std::size_t(y0)*analysis->width+x1,std::size_t(y1)*analysis->width+x0,std::size_t(y1)*analysis->width+x1};
                const double weights[]{(1-fx)*(1-fy),fx*(1-fy),(1-fx)*fy,fx*fy};fstr::SceneWeights roles{};
                for(auto&influence:influences)influence.weight=0;for(std::size_t c=0;c<stable.size();++c){stable[c].support=0;stable[c].strength=analysis->people[c].toning_strength;}
                for(std::size_t k=0;k<4;++k){for(std::size_t c=0;c<roles.size();++c)roles[c]+=float(weights[k]*analysis->scene[indices[k]][c]);for(std::size_t c=0;c<influences.size();++c){influences[c].weight+=weights[k]*analysis->people[c].mask[indices[k]];if(!analysis->people[c].toning_support.empty())stable[c].support+=weights[k]*analysis->people[c].toning_support[indices[k]];}}
                for(auto&weight:stable)weight.support=std::clamp(weight.support,0.,1.);
                if(!scene_residual)for(std::size_t c=1;c<roles.size();++c)roles[c]=0;
                auto prepared=prepare.apply(raw);for(std::size_t c=0;c<3;++c)prepared[c]=float(fstr::decode_host(prepared[c]/prepared[3],transfer));prepared[3]=1;
                const auto styled=scene->apply_stable_people(prepared,roles,influences,stable),plain=scene_plain->apply(prepared);
                for(std::size_t c=0;c<3;++c){const double delta=double(styled[c])-plain[c];if(delta==0)continue;const double linear=fstr::decode_host(v[c]/raw[3],transfer)+settings.mix*delta;const double value=fstr::encode_host(linear,transfer)*raw[3];if(!std::isfinite(value)||std::abs(value)>std::numeric_limits<float>::max())throw std::invalid_argument("Scene result exceeds finite range");v[c]=float(value);}
            }
            if constexpr(std::is_same_v<P,PF_PixelFloat>){dst[x]={src->alpha,v[0],v[1],v[2]};}
            else {
                auto quantize=[scale](float value){return static_cast<decltype(P::red)>(std::clamp(double(value),0.0,1.0)*scale+.5);};
                dst[x]={src->alpha,quantize(v[0]),quantize(v[1]),quantize(v[2])};
            }
        }
    }
    return PF_Err_NONE;
}
PF_Err render(PF_InData* in_data,PF_EffectWorld& input,PF_EffectWorld& output,bool smart,const std::vector<TemporalInput>& neighbors={},const std::vector<PreparationInput>& declared_anchors={}) {
    if(output.width==0||output.height==0)return PF_Err_NONE;
    Params params(in_data);const auto s=settings(params);const auto transfer=host_transfer(params);
    const auto primaries=static_cast<int>(transfer)<0?fstr::Primaries::Unknown:fstr::Primaries::SRGB_Rec709;
    const fstr::ColorProcessor validated(s,transfer,primaries,fstr::Alpha::Premultiplied);
    const bool auto_prepare=params[AutoPrepare].u.bd.value&&s.mix>0;
    if(auto_prepare&&(s.source!=fstr::SourceProfile::Host||static_cast<int>(transfer)<0))throw PreparationError("Auto Prepare requires normal SDR input");
    const bool enabled=params[SmartColor].u.bd.value&&s.look&&s.look_amount>0&&s.mix>0;
    struct LegacyAnchors {
        PF_InData* in;std::vector<PF_ParamDef> values;std::size_t acquired=0;
        PF_Err close()noexcept{PF_Err first=0;while(acquired){const auto e=PF_CHECKIN_PARAM(in,&values[--acquired]);if(!first)first=e;}return first;}
        ~LegacyAnchors(){close();}
    } legacy{in_data,{},0};
    std::vector<PreparationInput> anchors=declared_anchors;
    if(auto_prepare&&!smart){const auto times=fstr::sdr_clip_anchor_times(in_data->total_time,in_data->time_step);legacy.values.resize(times.size());for(std::size_t i=0;i<times.size();++i){auto e=PF_CHECKOUT_PARAM(in_data,Input,times[i],in_data->time_step,in_data->time_scale,&legacy.values[i]);if(e)throw e;++legacy.acquired;anchors.push_back({&legacy.values[i].u.ld,times[i]});}}

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
        if(!err&&enabled)for(const auto& n:neighbors){
            PF_PixelFormat nf{};
            if(!n.world){err=PF_Err_BAD_CALLBACK_PARAM;break;}
            err=world_suite->PF_GetPixelFormat(n.world,&nf);
            if(err||nf!=format){err=PF_Err_BAD_CALLBACK_PARAM;break;}
        }
        if(!err&&auto_prepare)for(const auto& anchor:anchors){PF_PixelFormat af{};if(!anchor.world){err=PF_Err_BAD_CALLBACK_PARAM;break;}err=world_suite->PF_GetPixelFormat(anchor.world,&af);if(err||af!=format){err=PF_Err_BAD_CALLBACK_PARAM;break;}}
        if(!err)switch(format) {
            case PF_PixelFormat_ARGB32:err=process_world<PF_Pixel>(in_data,input,output,smart,enabled,transfer,neighbors,s,params[Look].u.pd.value==9,auto_prepare,anchors);break;
            case PF_PixelFormat_ARGB64:err=process_world<PF_Pixel16>(in_data,input,output,smart,enabled,transfer,neighbors,s,params[Look].u.pd.value==9,auto_prepare,anchors);break;
            case PF_PixelFormat_ARGB128:err=process_world<PF_PixelFloat>(in_data,input,output,smart,enabled,transfer,neighbors,s,params[Look].u.pd.value==9,auto_prepare,anchors);break;
            default:err=PF_Err_BAD_CALLBACK_PARAM;
        }
    }catch(...){in_data->pica_basicP->ReleaseSuite(kPFWorldSuite,kPFWorldSuiteVersion2);throw;}
    const auto release=in_data->pica_basicP->ReleaseSuite(kPFWorldSuite,kPFWorldSuiteVersion2);
    const auto anchor_error=legacy.close(),checked=params.close();return err?err:(release?PF_Err_BAD_CALLBACK_PARAM:anchor_error?anchor_error:checked);
}
PF_Err pre_render(PF_InData* in_data,PF_PreRenderExtra* extra) {
    auto request=extra->input->output_request;request.channel_mask=PF_ChannelMask_ARGB;request.preserve_rgb_of_zero_alpha=TRUE;
    // Pre-render checked parameters are automatically checked in by AE.
    PF_ParamDef enabled{},look{},amount{},mix{},preparation{},source{};
    for(auto pair:{std::make_pair(SmartColor,&enabled),std::make_pair(Look,&look),std::make_pair(LookAmount,&amount),std::make_pair(Mix,&mix),std::make_pair(AutoPrepare,&preparation),std::make_pair(Source,&source)}){
        auto e=PF_CHECKOUT_PARAM(in_data,pair.first,in_data->current_time,in_data->time_step,in_data->time_scale,pair.second);if(e)return e;
    }
    const bool active=enabled.u.bd.value&&look.u.pd.value>1&&amount.u.fs_d.value>0&&mix.u.fs_d.value>0;
    const bool prepare_active=preparation.u.bd.value&&mix.u.fs_d.value>0;
    if(prepare_active&&source.u.pd.value!=1)throw PreparationError("Auto Prepare requires normal SDR");
    const auto analysis_cache=std::atomic_load(&scene_cache);const auto automatic=std::atomic_load(&automatic_cache);
    if(extra->cb->GuidMixInPtr){std::string guid=std::string("FSTR_NATIVE_BASE_V2/")+fstr::color_build_id+"/"+(active&&analysis_cache?analysis_cache->guid():active&&automatic?automatic->guid():"inactive");
        // The anchor dependency set is constant throughout a clip. Distinguish
        // which time supplies the output pixels, even when AE coalesces that set.
        // This salts host output caching only; the shot recipe remains reusable.
        if(prepare_active)guid+="/SDR_SHOT_V2_AE_TIME_V1/"+std::to_string(in_data->current_time)+"/"+std::to_string(in_data->time_scale)+"/"+std::to_string(in_data->time_step)+"/"+std::to_string(in_data->total_time);
        const auto error=extra->cb->GuidMixInPtr(in_data->effect_ref,static_cast<A_u_long>(guid.size()),guid.data());if(error)return error;}else if(prepare_active||(active&&(analysis_cache||automatic)))return PF_Err_BAD_CALLBACK_PARAM;
    PF_CheckoutResult result{};
    auto err=extra->cb->checkout_layer(in_data->effect_ref,Input,(active||prepare_active)?1000:Input,&request,in_data->current_time,in_data->time_step,in_data->time_scale,&result);
    if(err)return err;
    extra->output->result_rect=result.result_rect;extra->output->max_result_rect=result.max_result_rect;
    if(!active&&!prepare_active)return PF_Err_NONE;
    request.rect=result.max_result_rect;
    err=extra->cb->checkout_layer(in_data->effect_ref,Input,Input,&request,in_data->current_time,in_data->time_step,in_data->time_scale,&result);if(err)return err;
    auto data=std::make_unique<SmartPreData>();
    if(prepare_active){const auto times=fstr::sdr_clip_anchor_times(in_data->total_time,in_data->time_step);for(std::size_t i=0;i<times.size();++i){
        const int id=2000+int(i);auto anchor_request=request;PF_CheckoutResult anchor_bounds{};
        err=extra->cb->checkout_layer(in_data->effect_ref,Input,3000+int(i),&anchor_request,times[i],in_data->time_step,in_data->time_scale,&anchor_bounds);if(err)return err;
        anchor_request.rect=anchor_bounds.max_result_rect;
        err=extra->cb->checkout_layer(in_data->effect_ref,Input,id,&anchor_request,times[i],in_data->time_step,in_data->time_scale,&result);if(err)return err;data->anchors.emplace_back(id,times[i]);
    }}
    if(active&&in_data->time_step>0&&!analysis_cache)for(int offset:{-1,1,-2,2}){
        const auto time=std::int64_t(in_data->current_time)+std::int64_t(offset)*in_data->time_step;
        if(time<0||time>=in_data->total_time||time>INT32_MAX)continue;
        int id=offset==-1?1:offset==1?2:offset==-2?3:4;
        err=extra->cb->checkout_layer(in_data->effect_ref,Input,id,&request,static_cast<A_long>(time),in_data->time_step,in_data->time_scale,&result);if(err)return err;
        data->frames.emplace_back(id,offset);
    }
    extra->output->delete_pre_render_data_func=delete_smart_pre;
    extra->output->pre_render_data=data.release();return PF_Err_NONE;
}
PF_Err smart_render(PF_InData* in_data,PF_SmartRenderExtra* extra) {
    PF_EffectWorld *input=nullptr,*output=nullptr;
    std::vector<int> acquired;acquired.reserve(8);
    std::vector<TemporalInput> neighbors;neighbors.reserve(4);
    std::vector<PreparationInput> anchors;anchors.reserve(3);
    auto close=[&](){PF_Err first=PF_Err_NONE;for(auto i=acquired.rbegin();i!=acquired.rend();++i){auto e=extra->cb->checkin_layer_pixels(in_data->effect_ref,*i);if(!first)first=e;}acquired.clear();return first;};
    auto err=extra->cb->checkout_layer_pixels(in_data->effect_ref,Input,&input);if(err)return err;acquired.push_back(Input);
    try {
        if(extra->input&&extra->input->pre_render_data){
            for(const auto& f:static_cast<SmartPreData*>(extra->input->pre_render_data)->anchors){PF_EffectWorld* world=nullptr;err=extra->cb->checkout_layer_pixels(in_data->effect_ref,f.first,&world);if(err)break;acquired.push_back(f.first);anchors.push_back({world,f.second});}
            if(!err)
            for(const auto& f:static_cast<SmartPreData*>(extra->input->pre_render_data)->frames){
                PF_EffectWorld* world=nullptr;err=extra->cb->checkout_layer_pixels(in_data->effect_ref,f.first,&world);if(err)break;
                acquired.push_back(f.first);neighbors.push_back({world,f.second});
            }
        }
        if(!err)err=extra->cb->checkout_output(in_data->effect_ref,&output);
        if(!err)err=(!input||!output)?PF_Err_BAD_CALLBACK_PARAM:render(in_data,*input,*output,true,neighbors,anchors);
    }catch(...){close();throw;}
    const auto release=close();return err?err:release;
}
}
extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr data,PF_PluginDataCB2 callback,SPBasicSuite*,const char*,const char*) {
    PF_Err result=PF_Err_INVALID_CALLBACK;
    result=PF_REGISTER_EFFECT_EXT2(data,callback,"FSTR-LUT Development","FSTR LUT Color MVP","FSTR.Tools",AE_RESERVED_INFO,"EffectMain","https://github.com/ios3kov/FSTR_LUT");
    return result;
}
static PF_Err effect_main_impl(PF_Cmd cmd,PF_InData* in_data,PF_OutData* out_data,PF_ParamDef* params[],PF_LayerDef* output,void* extra) {
    try {
        switch(cmd) {
            case PF_Cmd_ABOUT:std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"FSTR-LUT | Build %s\rReady-to-use image: keep default input.\rCamera Log: match its profile in Advanced.\r3D: choose Linear RGB or ACEScg to match the render.\rUnknown profile? Check with the source creator.",fstr::color_build_id);break;
            case PF_Cmd_GLOBAL_SETUP:if(in_data->appl_id!=kAppID_AfterEffects)return PF_Err_INVALID_CALLBACK;initialize_look();initialize_scene_cache();initialize_automatic_analysis();initialize_preparation_cache();out_data->my_version=32769;out_data->out_flags=flags;out_data->out_flags2=flags2;break;
            case PF_Cmd_GLOBAL_SETDOWN:std::atomic_store(&preparation_cache,std::shared_ptr<fstr::SdrPreparationCache>{});std::atomic_store(&automatic_cache,std::shared_ptr<fstr::AutomaticSceneCache>{});std::atomic_store(&automatic_error,std::shared_ptr<const std::string>{});std::atomic_store(&scene_cache,std::shared_ptr<fstr::SceneCache>{});std::atomic_store(&looks,std::shared_ptr<const LookLibrary>{});fstr::ae::clear_all_metadata_cache();break;
            case PF_Cmd_SEQUENCE_SETUP:fstr::prepare_region_cache();fstr::ae::clear_metadata_cache(in_data->effect_ref);break;
            case PF_Cmd_SEQUENCE_SETDOWN:fstr::ae::clear_metadata_cache(in_data->effect_ref);break;
            // AE supplies the current flag words; never rewrite static capability bits here.
            // There are no parameter-dependent flag changes in this effect.
            case PF_Cmd_QUERY_DYNAMIC_FLAGS:break;
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
     catch(const PreparationError&){out_data->out_flags|=PF_OutFlag_DISPLAY_ERROR_MESSAGE;std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"Auto Prepare SDR could not run. Use normal SDR footage and fixed clip timing, or turn Auto Prepare SDR off.");return PF_Err_BAD_CALLBACK_PARAM;}
     catch(const SmartAnalysisError&){out_data->out_flags|=PF_OutFlag_DISPLAY_ERROR_MESSAGE;std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"Smart analysis could not run. Check the plugin model installation or turn Smart off.");return PF_Err_BAD_CALLBACK_PARAM;}
     catch(const fstr::UnsupportedColorProfile&){std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"Choose AE Pixel Encoding. Linearized project: select Linear. Sony override requires preserved, unconverted Log samples.");return PF_Err_BAD_CALLBACK_PARAM;}
     catch(...){std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"FSTR-LUT color processing failed; input, settings or range unsupported.");return PF_Err_BAD_CALLBACK_PARAM;}
}

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd,PF_InData* in_data,PF_OutData* out_data,PF_ParamDef* params[],PF_LayerDef* output,void* extra) {
    const auto trace=std::getenv("FSTR_LUT_SCENE_TRACE");const bool rendering=cmd==PF_Cmd_SMART_PRE_RENDER||cmd==PF_Cmd_SMART_RENDER||cmd==PF_Cmd_RENDER||cmd==PF_Cmd_QUERY_DYNAMIC_FLAGS;
    if(trace&&rendering)std::fprintf(stderr,"FSTR_NATIVE ENTER selector=%d\n",int(cmd));
    const auto err=effect_main_impl(cmd,in_data,out_data,params,output,extra);
    if(trace&&rendering)std::fprintf(stderr,"FSTR_NATIVE EXIT selector=%d code=%d\n",int(cmd),int(err));
    if(err&&std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_NATIVE ERROR selector=%d code=%d\n",int(cmd),int(err));
    return err;
}
