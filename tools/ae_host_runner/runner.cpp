#include "AEConfig.h"
#include "SPBasic.h"
#include "entry.h"
#include "AE_GeneralPlug.h"
#include "owned.hpp"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>

namespace {
// AEGP idle/entry callbacks execute on the host main thread. No render hook.
SPBasicSuite* basic=nullptr;
AEGP_PluginID plugin_id=0;
bool done=false;
std::chrono::steady_clock::time_point ready,deadline;
bool owns_process() noexcept {
    const auto value=std::getenv("FSTR_LUT_OWNED_TOKEN");
    return value&&std::strcmp(value,owned_nonce)==0;
}
#ifdef FSTR_RUNNER_TRACE_IDLE
void trace_rqm(const char*event){std::fprintf(stderr,"FSTR_RQM event=%s main=%d\n",event,pthread_main_np());}
A_Err job_start(AEGP_RQM_BasicData*,AEGP_RQM_SessionId){trace_rqm("job_start");return 0;}
A_Err job_end(AEGP_RQM_BasicData*,AEGP_RQM_SessionId){trace_rqm("job_end");return 0;}
A_Err item_start(AEGP_RQM_BasicData*,AEGP_RQM_SessionId,AEGP_RQM_ItemId){trace_rqm("item_start");return 0;}
A_Err item_update(AEGP_RQM_BasicData*,AEGP_RQM_SessionId,AEGP_RQM_ItemId,AEGP_RQM_FrameId){return 0;}
A_Err item_end(AEGP_RQM_BasicData*,AEGP_RQM_SessionId,AEGP_RQM_ItemId,AEGP_RQM_FinishedStatus){return 0;}
A_Err item_log(AEGP_RQM_BasicData*,AEGP_RQM_SessionId,AEGP_RQM_ItemId,A_Boolean,AEGP_MemHandle){return 0;}
const AEGP_RQM_FunctionBlock1 monitor{job_start,job_end,item_start,item_update,item_end,item_log};
#endif
A_Err idle(AEGP_GlobalRefcon,AEGP_IdleRefcon,A_long* max_sleep) noexcept {
    if(!owns_process())return A_Err_NONE;
#ifdef FSTR_RUNNER_TRACE_IDLE
    std::fprintf(stderr,"FSTR_IDLE main=%d after_dispatch=%d\n",pthread_main_np(),int(done));
#endif
    if(done)return A_Err_NONE;
    if(max_sleep&&*max_sleep>60)*max_sleep=60; // SDK units: 1/60 second.
    if(std::chrono::steady_clock::now()<ready)return A_Err_NONE;
    const AEGP_UtilitySuite6* utility=nullptr;
    const auto acquired=basic->AcquireSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6,reinterpret_cast<const void**>(&utility));
    if(acquired){done=true;std::fprintf(stderr,"FSTR_RUNNER acquire failed %ld\n",long(acquired));return A_Err_NONE;}
    A_Boolean available=FALSE;
    auto err=utility->AEGP_IsScriptingAvailable(&available);
    if(err||!available) {
        basic->ReleaseSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
        if(err||std::chrono::steady_clock::now()>deadline){done=true;std::fprintf(stderr,"FSTR_RUNNER scripting unavailable %ld\n",long(err));}
        return A_Err_NONE;
    }
    // Set before execution: do not retry after any unknown script effect.
    done=true;
    std::fprintf(stderr,"FSTR_RUNNER executing owned script\n");std::fflush(stderr);
    const AEGP_MemorySuite1* memory=nullptr;
    const auto mem_acquired=basic->AcquireSuite(kAEGPMemorySuite,kAEGPMemorySuiteVersion1,reinterpret_cast<const void**>(&memory));
    if(mem_acquired) {
        basic->ReleaseSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
        std::fprintf(stderr,"FSTR_RUNNER memory acquire failed %ld\n",long(mem_acquired));
        return A_Err_NONE;
    }
    AEGP_MemHandle error_text=nullptr;
    err=utility->AEGP_ExecuteScript(plugin_id,owned_script,FALSE,nullptr,&error_text);
    if(error_text) {
        AEGP_MemSize size=0; void* bytes=nullptr;
        const auto sized=memory->AEGP_GetMemHandleSize(error_text,&size);
        const auto locked=sized ? sized : memory->AEGP_LockMemHandle(error_text,&bytes);
        if(!locked) {
            if(bytes)std::fprintf(stderr,"FSTR_RUNNER script error: %.*s\n",int(size>4096?4096:size),static_cast<const char*>(bytes));
            const auto unlocked=memory->AEGP_UnlockMemHandle(error_text);
            std::fprintf(stderr,"FSTR_RUNNER error unlock %ld\n",long(unlocked));
        } else std::fprintf(stderr,"FSTR_RUNNER error read failed %ld\n",long(locked));
        const auto freed=memory->AEGP_FreeMemHandle(error_text);
        std::fprintf(stderr,"FSTR_RUNNER error free %ld\n",long(freed));
    }
    const auto mem_released=basic->ReleaseSuite(kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    const auto released=basic->ReleaseSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    std::fprintf(stderr,"FSTR_RUNNER return %ld release %ld memory release %ld\n",long(err),long(released),long(mem_released));std::fflush(stderr);
    return A_Err_NONE;
}
}
extern "C" DllExport A_Err EntryPointFunc(SPBasicSuite* suites,A_long,A_long,AEGP_PluginID id,AEGP_GlobalRefcon* global) {
    try {
        if(!owns_process())return A_Err_NONE; // Inactive in every other host process.
        if(!suites)return A_Err_GENERIC;
        basic=suites;plugin_id=id;done=false;
        ready=std::chrono::steady_clock::now()+std::chrono::seconds(2);
        deadline=ready+std::chrono::seconds(60);
        if(global)*global=nullptr;
        const AEGP_RegisterSuite5* registration=nullptr;
        const auto acquired=basic->AcquireSuite(kAEGPRegisterSuite,kAEGPRegisterSuiteVersion5,reinterpret_cast<const void**>(&registration));
        if(acquired)return acquired;
        const auto err=registration->AEGP_RegisterIdleHook(id,idle,nullptr);
        const auto released=basic->ReleaseSuite(kAEGPRegisterSuite,kAEGPRegisterSuiteVersion5);
        std::fprintf(stderr,"FSTR_RUNNER owned registration %ld release %ld\n",long(err),long(released));
#ifdef FSTR_RUNNER_TRACE_IDLE
        if(!err){const AEGP_RenderQueueMonitorSuite1*suite=nullptr;auto acquired_rqm=basic->AcquireSuite(kAEGPRenderQueueMonitorSuite,kAEGPRenderQueueMonitorSuiteVersion1,reinterpret_cast<const void**>(&suite));if(!acquired_rqm){auto registered=suite->AEGP_RegisterListener(id,nullptr,&monitor);auto released_rqm=basic->ReleaseSuite(kAEGPRenderQueueMonitorSuite,kAEGPRenderQueueMonitorSuiteVersion1);std::fprintf(stderr,"FSTR_RQM register=%ld release=%ld\n",long(registered),long(released_rqm));}else std::fprintf(stderr,"FSTR_RQM acquire=%ld\n",long(acquired_rqm));}
#endif
        // A successful registration publishes a callback; keep its image initialized.
        return err;
    }catch(...){return A_Err_GENERIC;}
}
