#include "sdr_timeline_host.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace {
PF_InData input{};SPBasicSuite basic{};PF_ParamUtilsSuite3 suite{};
int acquisitions=0,releases=0,builds=0;bool unavailable=false,comparison_error=false,cancelled=false;
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
SPErr acquire(const char*name,int32 version,const void**p){++acquisitions;check(!std::strcmp(name,kPFParamUtilsSuite)&&version==kPFParamUtilsSuiteVersion3,"Wrong suite");if(unavailable)return 76;*p=&suite;return 0;}
SPErr release(const char*,int32){++releases;return 0;}
PF_Err abort(PF_ProgPtr ref){check(ref==input.effect_ref,"Stale effect_ref in abort");return cancelled?75:0;}
PF_Err state(PF_ProgPtr ref,PF_ParamIndex id,const A_Time*a,const A_Time*b,PF_State*s){check(ref==input.effect_ref&&id==Input&&!a&&!b,"Not input-only all-time state");*s={};s->reservedAL[0]=7;return 0;}
PF_Err equal(PF_ProgPtr ref,const PF_State*a,const PF_State*b,A_Boolean*same){check(ref==input.effect_ref,"Cache retained old callback token");*same=a->reservedAL[0]==b->reservedAL[0];return comparison_error?77:0;}
fstr::ae::PreparedInput build(){++builds;return {{1,1,{1,2,3,255}},"identity-"+std::to_string(builds)};}
}
int main(){try{basic.AcquireSuite=acquire;basic.ReleaseSuite=release;input.pica_basicP=&basic;input.effect_ref=reinterpret_cast<PF_ProgPtr>(1);input.inter.abort=abort;suite.PF_GetCurrentState=state;suite.PF_AreStatesIdentical=equal;auto s=fstr::ae::sdr_input_state(&input);check(s.reservedAL[0]==7,"Wrong captured state");
 auto a=fstr::ae::get_prepared_input(&input,s,"frame/context",build);input.effect_ref=reinterpret_cast<PF_ProgPtr>(2);auto b=fstr::ae::get_prepared_input(&input,s,"frame/context",build);check(a==b&&builds==1,"Render clones did not share actual equal state");
 comparison_error=true;auto c=fstr::ae::get_prepared_input(&input,s,"frame/context",build);check(c!=b&&builds==2,"Comparison error reused stale pixels");comparison_error=false;
 int previous=builds;unavailable=true;auto x=fstr::ae::get_prepared_input(&input,s,"frame/context",build);check(builds==previous+1&&x!=a,"Unavailable suite skipped original preparation");unavailable=false;
 auto saved=suite.PF_AreStatesIdentical;suite.PF_AreStatesIdentical=nullptr;previous=builds;fstr::ae::get_prepared_input(&input,s,"frame/context",build);check(builds==previous+1,"Absent equality skipped original preparation");suite.PF_AreStatesIdentical=saved;
 cancelled=true;bool caught=false;try{fstr::ae::get_prepared_input(&input,s,"frame/context",build);}catch(PF_Err e){caught=e==75;}check(caught,"Host cancellation ignored");cancelled=false;
 fstr::ae::clear_prepared_inputs();previous=builds;fstr::ae::get_prepared_input(&input,s,"frame/context",build);check(builds==previous+1,"Setdown clear retained prepared pixels");fstr::ae::clear_prepared_inputs();check(releases==acquisitions-1,"Suite acquisition/release imbalance");std::cout<<"PASS actual SDK state suite, render clones, comparison failure, unsupported fallback, abort and clear\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}catch(PF_Err e){std::cerr<<"Unexpected PF_Err "<<e<<'\n';return 1;}}
