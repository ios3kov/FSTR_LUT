#include "sdr_timeline_host.hpp"
#include <mutex>
#include <map>
#include <cstdlib>
#include <cstdio>
namespace fstr::ae {namespace {
struct Suite {
    PF_InData* in;const PF_ParamUtilsSuite3* p=nullptr;
    explicit Suite(PF_InData*i):in(i){auto e=in->pica_basicP->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&p));if(e||!p||!p->PF_GetCurrentState||!p->PF_AreStatesIdentical){if(!e&&p)in->pica_basicP->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);}}
    ~Suite(){in->pica_basicP->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);}
};
struct Entry{PF_State state{};std::string metadata;std::shared_ptr<const HostSdrTimeline> value;};
// Input-only immutable results may be shared by AE render clones. effect_ref
// is an opaque callback token, not a stable cache owner or source identity.
std::mutex entries_mutex;std::vector<Entry> entries;
void abort(PF_InData*in){auto e=PF_ABORT(in);if(e)throw e;}
thread_local std::vector<PF_ProgPtr> computing;
}
const SdrShotRange& HostSdrTimeline::range(std::size_t frame)const{for(const auto&s:shots)if(frame>=s.begin&&frame<s.end)return s;throw std::invalid_argument("Frame outside complete scene map");}
const CachedSdrPreparation& HostSdrTimeline::at(std::size_t frame)const{for(std::size_t i=0;i<shots.size();++i)if(frame>=shots[i].begin&&frame<shots[i].end){if(i>=recipes.size())break;return recipes[i];}throw std::invalid_argument("Frame outside complete scene recipes");}
PF_State sdr_input_state(PF_InData*in){Suite suite(in);PF_State state{};const auto e=suite.p->PF_GetCurrentState(in->effect_ref,Input,nullptr,nullptr,&state);if(e)throw e;return state;}
std::shared_ptr<const HostSdrTimeline> get_sdr_timeline(PF_InData*in,const PF_State&state,const std::string&metadata,const std::function<std::shared_ptr<const HostSdrTimeline>()>&build){
    Suite suite(in);
    auto lookup=[&]()->std::shared_ptr<const HostSdrTimeline>{std::vector<Entry> snapshot;{std::lock_guard<std::mutex> lock(entries_mutex);snapshot=entries;}for(const auto&e:snapshot)if(e.metadata==metadata){A_Boolean same=FALSE;const auto error=suite.p->PF_AreStatesIdentical(in->effect_ref,&e.state,&state,&same);if(error)throw error;if(same)return e.value;}return {};};
    abort(in);if(auto value=lookup()){if(std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_SHOTS RAM shots=%zu\n",value->shots.size());return value;}
    for(auto ref:computing)if(ref==in->effect_ref)throw static_cast<PF_Err>(PF_Err_BAD_CALLBACK_PARAM);
    // Never wait for another render while making host callbacks. AE may need
    // that worker to satisfy a temporal checkout. Concurrent misses may compute
    // duplicates; only complete immutable results enter the bounded cache.
    computing.push_back(in->effect_ref);struct Pop{~Pop(){computing.pop_back();}}pop;
    if(std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_SHOTS BUILD_BEGIN\n");
    auto value=build();abort(in);if(!value||value->shots.empty()||value->shots.size()!=value->recipes.size())throw std::invalid_argument("Incomplete scene analysis");
    if(auto existing=lookup()){if(std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_SHOTS POSTBUILD_REUSE\n");return existing;}
    {std::lock_guard<std::mutex> lock(entries_mutex);if(entries.size()>=32)entries.erase(entries.begin());entries.push_back({state,metadata,value});}
    if(std::getenv("FSTR_LUT_SCENE_TRACE"))std::fprintf(stderr,"FSTR_SHOTS COMPUTE shots=%zu frames=%zu\n",value->shots.size(),value->shots.back().end);return value;
}
void clear_sdr_timelines(){std::lock_guard<std::mutex> lock(entries_mutex);entries.clear();}
}
