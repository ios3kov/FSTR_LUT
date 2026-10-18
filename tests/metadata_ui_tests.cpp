// Actual module entrypoint, owned SDK stand-ins, real bounded file parser.
#include "plugin.hpp"
#include "AE_EffectUI.h"
#include "AE_GeneralPlug.h"
#include "AE_EffectSuites.h"
#include "adobesdk/DrawbotSuite.h"
#include <chrono>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
int checks=0;PF_FilterProc effect=nullptr;
void check(bool value,const char* text){++checks;if(!value)throw std::runtime_error(text);}
struct Fixture {
    PF_InData in{};PF_OutData out{};SPBasicSuite basic{};
    AEGP_PFInterfaceSuite1 interface{};AEGP_LayerSuite9 layers{};AEGP_ItemSuite9 items{};AEGP_FootageSuite5 footage{};AEGP_MemorySuite1 memory{};
    PF_EffectCustomUISuite1 ui{};DRAWBOT_DrawbotSuite1 bot{};DRAWBOT_SupplierSuite1 supplier{};DRAWBOT_SurfaceSuite1 surface{};PFAppSuite4 app{};
    AEGP_ItemType type=AEGP_ItemType_FOOTAGE;AEGP_ItemFlags flags=0;bool fail_source=false,fail_free=false,odd_size=false,no_nul=false,fail_acquire=false;
    int acquisitions=0,releases=0,locks=0,unlocks=0,frees=0,objects=0;
    std::vector<A_UTF16Char> path;std::vector<std::string> text;
    Fixture();
    void set_path(const std::filesystem::path& value){const auto s=value.u16string();path.assign(s.begin(),s.end());path.push_back(0);}
};
thread_local Fixture* f=nullptr;
SPErr acquire(const char* name,int32 version,const void** out) {
    if(f->fail_acquire)return 17;
    ++f->acquisitions;
#define GET(NAME,VERSION,MEMBER) if(std::strcmp(name,NAME)==0&&version==VERSION){*out=&f->MEMBER;return 0;}
    GET(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,interface)
    GET(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,layers) GET(kAEGPItemSuite,kAEGPItemSuiteVersion9,items)
    GET(kAEGPFootageSuite,kAEGPFootageSuiteVersion5,footage) GET(kAEGPMemorySuite,kAEGPMemorySuiteVersion1,memory)
    GET(kPFEffectCustomUISuite,kPFEffectCustomUISuiteVersion1,ui)
    GET(kDRAWBOT_DrawSuite,kDRAWBOT_DrawSuite_Version1,bot) GET(kDRAWBOT_SupplierSuite,kDRAWBOT_SupplierSuite_Version1,supplier)
    GET(kDRAWBOT_SurfaceSuite,kDRAWBOT_SurfaceSuite_Version1,surface) GET(kPFAppSuite,kPFAppSuiteVersion4,app)
#undef GET
    return 18;
}
SPErr release(const char*,int32){++f->releases;return 0;}
A_Err layer(PF_ProgPtr,AEGP_LayerH* value){*value=reinterpret_cast<AEGP_LayerH>(f);return f->fail_source?19:0;}
A_Err item(AEGP_LayerH,AEGP_ItemH* value){*value=reinterpret_cast<AEGP_ItemH>(f);return 0;}
A_Err item_type(AEGP_ItemH,AEGP_ItemType* value){*value=f->type;return 0;}
A_Err item_flags(AEGP_ItemH,AEGP_ItemFlags* value){*value=f->flags;return 0;}
A_Err main_footage(AEGP_ItemH,AEGP_FootageH* value){*value=reinterpret_cast<AEGP_FootageH>(f);return 0;}
A_Err num_files(AEGP_FootageH,A_long* count,A_long* per){*count=*per=1;return 0;}
A_Err file_path(AEGP_FootageH,A_long,A_long,AEGP_MemHandle* value){*value=reinterpret_cast<AEGP_MemHandle>(f);return 0;}
A_Err size(AEGP_MemHandle,AEGP_MemSize* value){*value=f->path.size()*sizeof(A_UTF16Char)+(f->odd_size?1:0);return 0;}
A_Err lock(AEGP_MemHandle,void** value){++f->locks;*value=f->path.data();if(f->no_nul)f->path.back()='x';return 0;}
A_Err unlock(AEGP_MemHandle){++f->unlocks;return 0;}
A_Err free_mem(AEGP_MemHandle){++f->frees;return f->fail_free?20:0;}
PF_Err drawing(PF_ContextH,DRAWBOT_DrawRef* value){*value=reinterpret_cast<DRAWBOT_DrawRef>(f);return 0;}
SPErr get_supplier(DRAWBOT_DrawRef,DRAWBOT_SupplierRef* value){*value=reinterpret_cast<DRAWBOT_SupplierRef>(f);return 0;}
SPErr get_surface(DRAWBOT_DrawRef,DRAWBOT_SurfaceRef* value){*value=reinterpret_cast<DRAWBOT_SurfaceRef>(f);return 0;}
SPErr supports(DRAWBOT_SupplierRef,DRAWBOT_Boolean* value){*value=TRUE;return 0;}
SPErr font_size(DRAWBOT_SupplierRef,float* value){*value=12;return 0;}
SPErr font(DRAWBOT_SupplierRef,float,DRAWBOT_FontRef* value){*value=reinterpret_cast<DRAWBOT_FontRef>(f);++f->objects;return 0;}
SPErr brush(DRAWBOT_SupplierRef,const DRAWBOT_ColorRGBA*,DRAWBOT_BrushRef* value){*value=reinterpret_cast<DRAWBOT_BrushRef>(f);++f->objects;return 0;}
SPErr release_object(DRAWBOT_ObjectRef){--f->objects;return 0;}
SPErr paint(DRAWBOT_SurfaceRef,const DRAWBOT_ColorRGBA*,const DRAWBOT_RectF32*){return 0;}
SPErr draw_text(DRAWBOT_SurfaceRef,DRAWBOT_BrushRef,DRAWBOT_FontRef,const DRAWBOT_UTF16Char* value,const DRAWBOT_PointF32*,DRAWBOT_TextAlignment,DRAWBOT_TextTruncation,float) {
    std::string s;while(*value)s.push_back(static_cast<char>(*value++));f->text.push_back(s);return 0;
}
PF_Err background(PF_App_Color* value){*value={0,0,0};return 0;}
PF_Err color(PF_App_ColorType,PF_App_Color* value){*value={65535,65535,65535};return 0;}
Fixture::Fixture(){
    in.effect_ref=reinterpret_cast<PF_ProgPtr>(this);in.pica_basicP=&basic;
    basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    interface.AEGP_GetEffectLayer=layer;layers.AEGP_GetLayerSourceItem=item;items.AEGP_GetItemType=item_type;items.AEGP_GetItemFlags=item_flags;
    footage.AEGP_GetMainFootageFromItem=main_footage;footage.AEGP_GetFootageNumFiles=num_files;footage.AEGP_GetFootagePath=file_path;
    memory.AEGP_GetMemHandleSize=size;memory.AEGP_LockMemHandle=lock;memory.AEGP_UnlockMemHandle=unlock;memory.AEGP_FreeMemHandle=free_mem;
    ui.PF_GetDrawingReference=drawing;bot.GetSupplier=get_supplier;bot.GetSurface=get_surface;
    supplier.SupportsText=supports;supplier.GetDefaultFontSize=font_size;supplier.NewDefaultFont=font;supplier.NewBrush=brush;supplier.ReleaseObject=release_object;
    surface.PaintRect=paint;surface.DrawString=draw_text;app.PF_AppGetBgColor=background;app.PF_AppGetColor=color;
}
PF_Err event(Fixture& value,PF_EventType type=PF_Event_DRAW,int index=CameraInfo){
    f=&value;PF_EventExtra extra{};extra.e_type=type;extra.effect_win.index=index;extra.effect_win.area=PF_EA_CONTROL;extra.effect_win.current_frame={0,0,64,300};
    return effect(PF_Cmd_EVENT,&value.in,&value.out,nullptr,nullptr,&extra);
}
std::string box(const char* type,const std::string& data){unsigned n=static_cast<unsigned>(data.size()+8);std::string s;for(int shift=24;shift>=0;shift-=8)s.push_back(static_cast<char>((n>>shift)&255));return s+std::string(type,4)+data;}
}
int main(int argc,char** argv){try{
    check(argc==2,"Actual native module required");auto module=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);check(module!=nullptr,"dlopen failed");effect=reinterpret_cast<PF_FilterProc>(dlsym(module,"EffectMain"));check(effect!=nullptr,"Entry missing");
    const auto path=std::filesystem::temp_directory_path()/("fstr-ui-metadata-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mp4");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{path};
    const auto xml="<NonRealTimeMeta xmlns=\"urn:schemas-professionalDisc:nonRealTimeMeta:ver.2.20\"><Device manufacturer=\"Sony\" modelName=\"ILME-FX3\"/><AcquisitionRecord><Group name=\"CameraUnitMetadataSet\"><Item name=\"CaptureGammaEquation\" value=\"rec709\"/><Item name=\"CaptureColorPrimaries\" value=\"rec709\"/></Group></AcquisitionRecord></NonRealTimeMeta>";
    const auto bytes=box("ftyp","XAVC0000")+box("meta",std::string(4,'\0')+box("xml ",std::string(4,'\0')+xml));
    {std::ofstream out(path,std::ios::binary);out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));}
    Fixture normal;normal.set_path(path);check(event(normal)==0,"Metadata DRAW failed");
    check(normal.text.size()==3&&normal.text[0]=="Sony ILME-FX3"&&normal.text[1]=="Rec.709 (SDR)","Declared source display wrong");
    check(normal.acquisitions==normal.releases&&normal.locks==1&&normal.unlocks==1&&normal.frees==1&&normal.objects==0,"UI resources unbalanced");
    std::filesystem::remove(path);normal.text.clear();check(event(normal)==0&&normal.text[0]=="Sony ILME-FX3","Cached redraw reread source");
    check(event(normal,PF_Event_DO_CLICK)==0,"Refresh click failed");normal.text.clear();check(event(normal)==0&&normal.text[0]=="Source file unavailable","Missing source after refresh was hidden");
    for(int mode=0;mode<8;++mode){Fixture bad;bad.set_path(path);if(mode==0)bad.flags=AEGP_ItemFlag_USING_PROXY;if(mode==1)bad.flags=AEGP_ItemFlag_MISSING;if(mode==2)bad.type=AEGP_ItemType_COMP;if(mode==3)bad.fail_source=true;if(mode==4)bad.odd_size=true;if(mode==5)bad.no_nul=true;if(mode==6)bad.fail_free=true;if(mode==7)bad.path[0]=0xD800;
        check(event(bad)==0,"Manual fallback DRAW failed");check(bad.text.size()==3&&bad.text[1]=="Normal image? Keep the default input."&&bad.text[2]=="Log or 3D? Choose Input Profile in Advanced.","Fallback missing");check(bad.acquisitions==bad.releases&&bad.locks==bad.unlocks&&bad.objects==0,"Fallback cleanup unbalanced");}
    Fixture ignored;check(event(ignored,PF_Event_DRAW,WorkingSpace)==0&&ignored.acquisitions==0,"Unrelated UI queried metadata");
    f=&normal;check(effect(PF_Cmd_GLOBAL_SETDOWN,&normal.in,&normal.out,nullptr,nullptr,nullptr)==0,"Cache teardown failed");check(dlclose(module)==0,"Module cleanup failed");
    std::cout<<"PASS "<<checks<<" owned metadata UI assertions; actual AE display NOT RUN\n";return 0;
}catch(const std::exception& error){std::cerr<<"FAIL "<<error.what()<<" assertion "<<checks<<'\n';return 1;}}
