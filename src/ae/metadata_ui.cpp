#include "metadata_ui.hpp"
#include "fstr/metadata.hpp"
#include "AE_GeneralPlug.h"
#include "AE_EffectSuites.h"
#include "adobesdk/DrawbotSuite.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fstr::ae {
namespace {
// UI only. Never queried by a render callback or stored in render sequence data.
struct Cached {
    std::filesystem::path path;std::filesystem::file_time_type stamp{};
    std::uintmax_t size=0;CameraMetadata value;
    std::chrono::steady_clock::time_point checked{};
};
std::mutex cache_mutex;
std::unordered_map<PF_ProgPtr,Cached> cache;
class Suites {
    SPBasicSuite* basic_;std::vector<std::pair<const char*,int>> acquired_;
public:
    explicit Suites(SPBasicSuite* basic):basic_(basic){if(!basic_)throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);acquired_.reserve(10);}
    template<class T>const T* get(const char* name,int version) {
        const T* value=nullptr;
        if(basic_->AcquireSuite(name,version,reinterpret_cast<const void**>(&value)))throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
        acquired_.emplace_back(name,version);if(!value)throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);return value;
    }
    PF_Err close() noexcept {PF_Err first=0;while(!acquired_.empty()){const auto item=acquired_.back();acquired_.pop_back();const auto error=basic_->ReleaseSuite(item.first,item.second);if(error&&!first)first=PF_Err_BAD_CALLBACK_PARAM;}return first;}
    ~Suites(){close();}
};
struct Memory {
    const AEGP_MemorySuite1* suite;AEGP_MemHandle handle=nullptr;bool locked=false;
    PF_Err close() noexcept {
        PF_Err err=0;if(locked){if(suite->AEGP_UnlockMemHandle(handle))err=PF_Err_BAD_CALLBACK_PARAM;locked=false;}
        if(handle){if(suite->AEGP_FreeMemHandle(handle))err=PF_Err_BAD_CALLBACK_PARAM;handle=nullptr;}return err;
    }
    ~Memory(){close();}
};
CameraMetadata unavailable(const char* message) {CameraMetadata value;value.status=MetadataStatus::Unsupported;value.message=message;return value;}
std::pair<std::filesystem::path,CameraMetadata> source(PF_InData* in) {
    Suites suites(in->pica_basicP);
    const auto* interface=suites.get<AEGP_PFInterfaceSuite1>(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    const auto* layers=suites.get<AEGP_LayerSuite9>(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    const auto* items=suites.get<AEGP_ItemSuite9>(kAEGPItemSuite,kAEGPItemSuiteVersion9);
    const auto* footage=suites.get<AEGP_FootageSuite5>(kAEGPFootageSuite,kAEGPFootageSuiteVersion5);
    const auto* memory=suites.get<AEGP_MemorySuite1>(kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    AEGP_LayerH layer=nullptr;AEGP_ItemH item=nullptr;AEGP_FootageH file=nullptr;
    AEGP_ItemType type{};AEGP_ItemFlags flags{};A_long files=0,per_frame=0;
    std::filesystem::path path;CameraMetadata value=unavailable("Source metadata unavailable");
    auto err=interface->AEGP_GetEffectLayer(in->effect_ref,&layer);
    if(!err&&layer)err=layers->AEGP_GetLayerSourceItem(layer,&item);
    if(!err&&item)err=items->AEGP_GetItemType(item,&type);
    if(!err&&item)err=items->AEGP_GetItemFlags(item,&flags);
    if(!err&&type!=AEGP_ItemType_FOOTAGE)value=unavailable("Composition or generated source: choose profile manually");
    else if(!err&&(flags&AEGP_ItemFlag_USING_PROXY))value=unavailable("Proxy active: original camera metadata does not describe this input");
    else if(!err&&(flags&AEGP_ItemFlag_MISSING))value=unavailable("Source file is missing");
    else if(!err&&item) {
        err=footage->AEGP_GetMainFootageFromItem(item,&file);
        if(!err&&file)err=footage->AEGP_GetFootageNumFiles(file,&files,&per_frame);
        if(!err&&(files!=1||per_frame!=1))value=unavailable("Sequence or multi-file source: choose profile manually");
        else if(!err&&file) {
            Memory data{memory};err=footage->AEGP_GetFootagePath(file,0,AEGP_FOOTAGE_MAIN_FILE_INDEX,&data.handle);
            AEGP_MemSize size=0;void* bytes=nullptr;
            if(!err&&data.handle)err=memory->AEGP_GetMemHandleSize(data.handle,&size);
            if(!err&&(size<sizeof(A_UTF16Char)||size>32768||size%sizeof(A_UTF16Char)))err=PF_Err_BAD_CALLBACK_PARAM;
            if(!err){err=memory->AEGP_LockMemHandle(data.handle,&bytes);data.locked=!err;}
            if(!err&&bytes) {
                const auto* chars=static_cast<const A_UTF16Char*>(bytes);const auto count=static_cast<std::size_t>(size/sizeof(A_UTF16Char));
                const auto end=std::find(chars,chars+count,A_UTF16Char(0));
                if(end==chars+count)err=PF_Err_BAD_CALLBACK_PARAM;
                else if(end!=chars) {
                    for(auto* c=chars;c!=end&&!err;++c) {
                        if(*c>=0xD800&&*c<=0xDBFF){if(c+1==end||c[1]<0xDC00||c[1]>0xDFFF)err=PF_Err_BAD_CALLBACK_PARAM;else ++c;}
                        else if(*c>=0xDC00&&*c<=0xDFFF)err=PF_Err_BAD_CALLBACK_PARAM;
                    }
                    if(!err)path=std::filesystem::path(std::u16string(chars,end));
                }
                else value=unavailable("Generated or placeholder source: no camera file");
            } else if(!err)err=PF_Err_BAD_CALLBACK_PARAM;
            const auto cleanup=data.close();if(!err)err=cleanup;
        }
    }
    const auto released=suites.close();if(err||released){path.clear();value=unavailable("AE source metadata could not be read");}
    return {std::move(path),std::move(value)};
}
CameraMetadata cached_metadata(PF_InData* in) {
    // GetEffectLayer is documented here for EVENT::DRAW, not UPDATE_PARAMS_UI.
    std::pair<std::filesystem::path,CameraMetadata> discovered;
    try {discovered=source(in);}catch(PF_Err){return unavailable("AE source metadata lookup unavailable");}
    if(discovered.first.empty())return discovered.second;
    const auto now=std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(cache_mutex);const auto found=cache.find(in->effect_ref);
        if(found!=cache.end()&&found->second.path==discovered.first&&now-found->second.checked<std::chrono::seconds(2))return found->second.value;
    }
    std::error_code error;const auto stamp=std::filesystem::last_write_time(discovered.first,error);
    if(error)return unavailable("Source file unavailable");
    const auto size=std::filesystem::file_size(discovered.first,error);if(error)return unavailable("Source file unavailable");
    {
        std::lock_guard<std::mutex> lock(cache_mutex);const auto found=cache.find(in->effect_ref);
        if(found!=cache.end()&&found->second.path==discovered.first&&found->second.stamp==stamp&&found->second.size==size){found->second.checked=now;return found->second.value;}
    }
    auto value=read_camera_metadata(discovered.first);
    {std::lock_guard<std::mutex> lock(cache_mutex);if(cache.size()>=128)cache.clear();cache[in->effect_ref]={discovered.first,stamp,size,value,now};}
    return value;
}
std::vector<DRAWBOT_UTF16Char> ascii_display(const std::string& value) {
    std::vector<DRAWBOT_UTF16Char> out;out.reserve(value.size()+1);
    for(unsigned char c:value)out.push_back(static_cast<DRAWBOT_UTF16Char>(c>=32&&c<127?c:'?'));out.push_back(0);return out;
}
struct DrawObjects {
    const DRAWBOT_SupplierSuite1* supplier;
    DRAWBOT_FontRef font=nullptr;DRAWBOT_BrushRef brush=nullptr;
    SPErr close() noexcept {
        SPErr first=0;
        if(brush){first=supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));brush=nullptr;}
        if(font){const auto err=supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(font));if(!first)first=err;font=nullptr;}
        return first;
    }
    ~DrawObjects(){close();}
};
PF_Err draw(PF_InData* in,PF_EventExtra* extra,const CameraMetadata& value) {
    Suites suites(in->pica_basicP);
    const auto* ui=suites.get<PF_EffectCustomUISuite1>(kPFEffectCustomUISuite,kPFEffectCustomUISuiteVersion1);
    const auto* bot=suites.get<DRAWBOT_DrawbotSuite1>(kDRAWBOT_DrawSuite,kDRAWBOT_DrawSuite_Version1);
    const auto* supplier=suites.get<DRAWBOT_SupplierSuite1>(kDRAWBOT_SupplierSuite,kDRAWBOT_SupplierSuite_Version1);
    const auto* surface=suites.get<DRAWBOT_SurfaceSuite1>(kDRAWBOT_SurfaceSuite,kDRAWBOT_SurfaceSuite_Version1);
    const auto* app=suites.get<PFAppSuite4>(kPFAppSuite,kPFAppSuiteVersion4);
    DRAWBOT_DrawRef reference=nullptr;DRAWBOT_SupplierRef supply=nullptr;DRAWBOT_SurfaceRef canvas=nullptr;
    DrawObjects objects{supplier};float font_size=12;
    PF_App_Color background{},text{};DRAWBOT_Boolean supports=FALSE;
    auto err=ui->PF_GetDrawingReference(extra->contextH,&reference);
    if(!err)err=bot->GetSupplier(reference,&supply);if(!err)err=bot->GetSurface(reference,&canvas);
    if(!err)err=supplier->SupportsText(supply,&supports);if(!err&&!supports)err=PF_Err_BAD_CALLBACK_PARAM;
    if(!err)err=app->PF_AppGetBgColor(&background);if(!err)err=app->PF_AppGetColor(PF_App_Color_TEXT,&text);
    const auto color=[](const PF_App_Color& c){return DRAWBOT_ColorRGBA{c.red/65535.f,c.green/65535.f,c.blue/65535.f,1};};
    const auto bg=color(background),fg=color(text);const auto rect=extra->effect_win.current_frame;
    DRAWBOT_RectF32 area{float(rect.left),float(rect.top),float(rect.right-rect.left),float(rect.bottom-rect.top)};
    if(!err)err=surface->PaintRect(canvas,&bg,&area);
    if(!err)err=supplier->GetDefaultFontSize(supply,&font_size);
    if(!err)err=supplier->NewDefaultFont(supply,font_size,&objects.font);
    if(!err)err=supplier->NewBrush(supply,&fg,&objects.brush);
    std::array<std::string,3> lines;
    if(value.status==MetadataStatus::Found) {
        lines[0]=(value.manufacturer.empty()?"Camera":value.manufacturer)+(value.model.empty()?"":" "+value.model);
        lines[1]=value.gamma.empty()||value.primaries.empty()?"Color profile not declared":std::string(declared_profile_name(value.profile));
        if(value.profile==DeclaredProfile::Other)lines[1]=value.gamma+" / "+value.primaries;
        lines[2]=value.profile==DeclaredProfile::Rec709?"Suggested input: Already converted / SDR":"Log source? Match its profile in Advanced";
    } else {
        lines[0]=value.status==MetadataStatus::NotFound?"No camera metadata in source":value.message;
        lines[1]="Normal image? Keep the default input.";
        lines[2]="Log or 3D? Choose Input Profile in Advanced.";
    }
    for(std::size_t i=0;i<lines.size()&&!err;++i) {
        const auto string=ascii_display(lines[i]);DRAWBOT_PointF32 origin{area.left+5,area.top+font_size+4+float(i)*(font_size+5)};
        err=surface->DrawString(canvas,objects.brush,objects.font,string.data(),&origin,kDRAWBOT_TextAlignment_Left,kDRAWBOT_TextTruncation_EndEllipsis,std::max(0.f,area.width-10));
    }
    const auto objects_released=objects.close();if(!err)err=objects_released;
    const auto released=suites.close();return err?static_cast<PF_Err>(err):released;
}
}
PF_Err metadata_event(PF_InData* in,PF_EventExtra* extra) {
    if(!extra||extra->effect_win.index!=CameraInfo||extra->effect_win.area!=PF_EA_CONTROL)return PF_Err_NONE;
    if(extra->e_type==PF_Event_DO_CLICK){clear_metadata_cache(in->effect_ref);extra->evt_out_flags|=PF_EO_HANDLED_EVENT|PF_EO_UPDATE_NOW;return PF_Err_NONE;}
    if(extra->e_type!=PF_Event_DRAW)return PF_Err_NONE;
    const auto result=draw(in,extra,cached_metadata(in));if(!result)extra->evt_out_flags|=PF_EO_HANDLED_EVENT;return result;
}
void clear_metadata_cache(PF_ProgPtr effect){std::lock_guard<std::mutex> lock(cache_mutex);cache.erase(effect);}
void clear_all_metadata_cache(){std::lock_guard<std::mutex> lock(cache_mutex);cache.clear();}
}
