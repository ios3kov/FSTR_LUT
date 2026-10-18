#include "external_lut_ui.hpp"
#include "fstr/external_lut.hpp"
#include "fstr_embedded_look.hpp"
#include "json.hpp"
#include <cstring>
#include <map>
#include <filesystem>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include "AE_EffectSuites.h"
namespace fstr::ae {namespace {
using Json=nlohmann::json;
constexpr std::size_t reference_budget=32768,resource_budget=128*1024*1024;
struct Entry {ExternalLookValue value;std::size_t refs=1,bytes=0;};
std::map<PF_ProgPtr,A_short> selections;
std::mutex mutex;std::map<std::uint64_t,Entry> entries;std::uint64_t next_token=1;std::size_t total_bytes=0;
std::uint64_t token(PF_InData* in_data,PF_ArbitraryH h){
    if(!h)return 0;if(PF_GET_HANDLE_SIZE(h)!=sizeof(std::uint64_t))throw std::invalid_argument("Invalid LUT reference handle");
    auto* p=PF_LOCK_HANDLE(h);if(!p)throw std::bad_alloc();std::uint64_t id;std::memcpy(&id,p,sizeof(id));PF_UNLOCK_HANDLE(h);return id;
}
void release_locked(std::uint64_t id){if(!id)return;auto it=entries.find(id);if(it!=entries.end()&&--it->second.refs==0){total_bytes-=it->second.bytes;entries.erase(it);}}
void release(std::uint64_t id){std::lock_guard<std::mutex> guard(mutex);release_locked(id);}
PF_ArbitraryH handle(PF_InData* in_data,std::uint64_t id){
    auto h=PF_NEW_HANDLE(sizeof(id));if(!h)throw std::bad_alloc();auto* p=PF_LOCK_HANDLE(h);if(!p){PF_DISPOSE_HANDLE(h);throw std::bad_alloc();}std::memcpy(p,&id,sizeof(id));PF_UNLOCK_HANDLE(h);return h;
}
std::uint64_t load(const std::string& path,const std::string& fingerprint,bool restoring){
    if(path.empty())return 0;if(path.size()>4095||path.find('\0')!=std::string::npos||!std::filesystem::path(path).is_absolute())throw std::invalid_argument("Invalid external LUT path");
    ExternalLookValue value;value.path=path;value.identity=fingerprint;std::size_t bytes=0;
    try{const auto file=load_external_lut(path);value.look=file.look;value.identity=file.identity;bytes=std::max<std::size_t>(4*1024*1024,file.bytes);}catch(const std::bad_alloc&){throw;}catch(const std::exception&){if(!restoring)throw;value.error="External LUT file is missing or invalid";}
    std::lock_guard<std::mutex> guard(mutex);
    for(auto& item:entries)if(item.second.value.path==path&&item.second.value.identity==value.identity&&item.second.value.error==value.error){++item.second.refs;return item.first;}
    if(entries.size()>=64||bytes>resource_budget-total_bytes||next_token==0)throw std::runtime_error("External LUT memory budget exceeded");
    const auto id=next_token++;entries.emplace(id,Entry{std::move(value),1,bytes});total_bytes+=bytes;return id;
}
PF_ArbitraryH own(PF_InData* in_data,std::uint64_t id){try{return handle(in_data,id);}catch(...){release(id);throw;}}
PF_ArbitraryH copy(PF_InData* in_data,PF_ArbitraryH source){std::uint64_t id;{std::lock_guard<std::mutex> guard(mutex);id=token(in_data,source);if(id){auto it=entries.find(id);if(it==entries.end())throw std::invalid_argument("Invalid LUT reference copy");++it->second.refs;}}auto result=own(in_data,id);if(std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_LUT_ARB COPY token=%llu src=%p dst=%p\n",static_cast<unsigned long long>(id),static_cast<void*>(source),static_cast<void*>(result));return result;}
std::string flatten(PF_InData* in_data,PF_ArbitraryH source){const auto value=external_reference(in_data,source);return Json{{"schema",1},{"path",value.path},{"fingerprint",value.identity}}.dump();}
PF_ArbitraryH unflatten(PF_InData* in_data,const char* p,std::size_t n){
    if(!p||!n||n>reference_budget)throw std::invalid_argument("Invalid LUT reference size");const auto j=Json::parse(p,p+n);if(j.at("schema")!=1)throw std::invalid_argument("Unknown LUT reference version");const auto path=j.at("path").get<std::string>(),fingerprint=j.at("fingerprint").get<std::string>();
    if(!fingerprint.empty()&&(fingerprint.size()!=64||!std::all_of(fingerprint.begin(),fingerprint.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');})))throw std::invalid_argument("Invalid LUT revision");
    return own(in_data,load(path,fingerprint,true));
}
}
PF_ArbitraryH new_external_reference(PF_InData* in_data){return handle(in_data,0);}
ExternalLookValue external_reference(PF_InData* in_data,PF_ArbitraryH h){std::lock_guard<std::mutex> guard(mutex);const auto id=token(in_data,h);if(!id)return {};const auto it=entries.find(id);if(it==entries.end())throw std::invalid_argument("Invalid LUT reference token");return it->second.value;}
PF_Err refresh_external_menu(PF_InData* in,PF_ParamDef* params[]){
    const auto value=external_reference(in,params[LUTReference]->u.arb_d.value);std::string label="Custom";
    if(!value.path.empty()){auto name=display_lut_name(std::filesystem::path(value.path).filename().string());for(auto& c:name)if(c=='|'||static_cast<unsigned char>(c)<32)c=' ';if(name.size()>160){std::size_t end=157;while(end&&((static_cast<unsigned char>(name[end])&0xc0)==0x80))--end;name.resize(end);name+="...";}label=(value.error.empty()?"Custom: ":"Missing: ")+name;}
    const std::string menu=std::string(fstr::builtin_look_menu)+"|"+label+"|Load LUT...";PF_ParamDef ui=*params[Look];ui.u.pd.u.PF_DEF_NAMESPTR=menu.c_str();
    const PF_ParamUtilsSuite3* suite=nullptr;const auto acquired=in->pica_basicP->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&suite));if(acquired||!suite)return PF_Err_BAD_CALLBACK_PARAM;
    const auto error=suite->PF_UpdateParamUI(in->effect_ref,Look,&ui);const auto released=in->pica_basicP->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);return error?error:(released?PF_Err_BAD_CALLBACK_PARAM:PF_Err_NONE);
}
void remember_external_selection(PF_InData* in,int look){if(look<1||look>static_cast<int>(builtin_look_count+1))return;std::lock_guard<std::mutex> guard(mutex);selections[in->effect_ref]=static_cast<A_short>(look);}
A_short previous_external_selection(PF_InData* in){std::lock_guard<std::mutex> guard(mutex);const auto it=selections.find(in->effect_ref);return it==selections.end()?1:it->second;}
void forget_external_selection(PF_InData* in){std::lock_guard<std::mutex> guard(mutex);selections.erase(in->effect_ref);}
void clear_external_references(){std::lock_guard<std::mutex> guard(mutex);entries.clear();selections.clear();total_bytes=0;}
PF_Err external_arbitrary(PF_InData* in_data,PF_ArbParamsExtra* e){
    if(!e||e->id!=ExternalLUTId)return PF_Err_BAD_CALLBACK_PARAM;
    switch(e->which_function){
        case PF_Arbitrary_NEW_FUNC:*e->u.new_func_params.arbPH=new_external_reference(in_data);break;
        case PF_Arbitrary_DISPOSE_FUNC:{auto h=e->u.dispose_func_params.arbH;{std::lock_guard<std::mutex> guard(mutex);release_locked(token(in_data,h));}if(h)PF_DISPOSE_HANDLE(h);break;}
        case PF_Arbitrary_COPY_FUNC:*e->u.copy_func_params.dst_arbPH=copy(in_data,e->u.copy_func_params.src_arbH);break;
        case PF_Arbitrary_FLAT_SIZE_FUNC:*e->u.flat_size_func_params.flat_data_sizePLu=static_cast<A_u_long>(flatten(in_data,e->u.flat_size_func_params.arbH).size());break;
        case PF_Arbitrary_FLATTEN_FUNC:{const auto& p=e->u.flatten_func_params;const auto s=flatten(in_data,p.arbH);if(!p.flat_dataPV||p.buf_sizeLu<s.size())return PF_Err_BAD_CALLBACK_PARAM;std::memcpy(p.flat_dataPV,s.data(),s.size());break;}
        case PF_Arbitrary_UNFLATTEN_FUNC:{auto& p=e->u.unflatten_func_params;*p.arbPH=unflatten(in_data,static_cast<const char*>(p.flat_dataPV),p.buf_sizeLu);break;}
        case PF_Arbitrary_INTERP_FUNC:{auto& p=e->u.interp_func_params;*p.interpPH=copy(in_data,p.tF<1?p.left_arbH:p.right_arbH);break;}
        case PF_Arbitrary_COMPARE_FUNC:{auto& p=e->u.compare_func_params;*p.compareP=flatten(in_data,p.a_arbH)==flatten(in_data,p.b_arbH)?PF_ArbCompare_EQUAL:PF_ArbCompare_NOT_EQUAL;if(std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_LUT_ARB COMPARE a=%llu b=%llu result=%d\n",static_cast<unsigned long long>(token(in_data,p.a_arbH)),static_cast<unsigned long long>(token(in_data,p.b_arbH)),int(*p.compareP));break;}
        case PF_Arbitrary_PRINT_SIZE_FUNC:*e->u.print_size_func_params.print_sizePLu=static_cast<A_u_long>(flatten(in_data,e->u.print_size_func_params.arbH).size()+1);break;
        case PF_Arbitrary_PRINT_FUNC:{const auto& p=e->u.print_func_params;const auto s=flatten(in_data,p.arbH);if(!p.print_bufferPC||p.print_sizeLu<s.size()+1)return PF_Err_BAD_CALLBACK_PARAM;std::memcpy(p.print_bufferPC,s.c_str(),s.size()+1);break;}
        case PF_Arbitrary_SCAN_FUNC:{auto& p=e->u.scan_func_params;*p.arbPH=unflatten(in_data,p.bufPC,p.bytes_to_scanLu);break;}
        default:return PF_Err_BAD_CALLBACK_PARAM;
    }return PF_Err_NONE;
}
PF_Err import_external_lut(PF_InData* in_data,PF_OutData* out,PF_ParamDef* params[]){
    const auto old=external_reference(in_data,params[LUTReference]->u.arb_d.value);const auto path=choose_lut_file(old.path);if(path.empty()){params[Look]->u.pd.value=previous_external_selection(in_data);params[Look]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;out->out_flags|=PF_OutFlag_REFRESH_UI;return PF_Err_NONE;}
    const auto id=load(path,{},false);auto h=params[LUTReference]->u.arb_d.value;
    if(h){try{std::lock_guard<std::mutex> guard(mutex);const auto old_id=token(in_data,h);if(std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_LUT_ARB COMMIT old=%llu new=%llu handle=%p\n",static_cast<unsigned long long>(old_id),static_cast<unsigned long long>(id),static_cast<void*>(h));auto* p=PF_LOCK_HANDLE(h);if(!p)throw std::bad_alloc();std::memcpy(p,&id,sizeof(id));PF_UNLOCK_HANDLE(h);release_locked(old_id);}catch(...){release(id);throw;}}
    else params[LUTReference]->u.arb_d.value=own(in_data,id);
    params[LUTReference]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;params[Look]->u.pd.value=static_cast<A_short>(builtin_look_count+1);params[Look]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;out->out_flags|=PF_OutFlag_REFRESH_UI;return PF_Err_NONE;
}
}
