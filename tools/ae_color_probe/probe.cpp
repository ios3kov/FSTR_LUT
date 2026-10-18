#define PF_DEEP_COLOR_AWARE 1
#include "AEConfig.h"
#include "SPBasic.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_EffectSuites.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <new>
#include <pthread.h>
namespace {
constexpr int Mode=7,Count=8;
#ifdef FSTR_COLOR_LIFECYCLE
PF_Err sequence(PF_Cmd cmd,PF_InData*in_data,PF_OutData*out){
 if(cmd==PF_Cmd_SEQUENCE_SETDOWN){if(in_data->sequence_data)PF_DISPOSE_HANDLE(in_data->sequence_data);out->sequence_data=nullptr;return 0;}
 if(cmd==PF_Cmd_SEQUENCE_FLATTEN){out->sequence_data=in_data->sequence_data;return 0;}
 if(cmd==PF_Cmd_SEQUENCE_RESETUP&&in_data->sequence_data){out->sequence_data=in_data->sequence_data;return 0;}
 auto handle=PF_NEW_HANDLE(sizeof(A_long));if(!handle)return PF_Err_OUT_OF_MEMORY;auto data=static_cast<A_long*>(PF_LOCK_HANDLE(handle));if(!data){PF_DISPOSE_HANDLE(handle);return PF_Err_OUT_OF_MEMORY;}*data=20261018;PF_UNLOCK_HANDLE(handle);out->sequence_data=handle;return 0;
}
#endif
PF_Err params(PF_InData* in_data,PF_OutData*out_data){PF_ParamDef def{};const char* names[]{"Gray32","Gray128","Gray224","Red","Green","Blue"};const int colors[6][3]{{32,32,32},{128,128,128},{224,224,224},{255,0,0},{0,255,0},{0,0,255}};for(int i=0;i<6;++i){AEFX_CLR_STRUCT(def);def.ui_flags=PF_PUI_INVISIBLE;PF_ADD_COLOR(names[i],colors[i][0],colors[i][1],colors[i][2],100+i);}AEFX_CLR_STRUCT(def);def.ui_flags=PF_PUI_INVISIBLE;PF_ADD_POPUP("Color Probe Mode",16,1,"1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16",106);out_data->num_params=Count;return 0;}
PF_Err values(PF_InData*in){const PF_ColorParamSuite1*suite=nullptr;if(in->pica_basicP->AcquireSuite(kPFColorParamSuite,kPFColorParamSuiteVersion1,reinterpret_cast<const void**>(&suite))||!suite)return PF_Err_BAD_CALLBACK_PARAM;PF_Err error=0;PF_ParamDef mode{};error=PF_CHECKOUT_PARAM(in,Mode,in->current_time,in->time_step,in->time_scale,&mode);if(!error){int stage=mode.u.pd.value;error=PF_CHECKIN_PARAM(in,&mode);for(int i=1;i<=6&&!error;++i){PF_ParamDef def{};error=PF_CHECKOUT_PARAM(in,i,in->current_time,in->time_step,in->time_scale,&def);if(error)break;PF_PixelFloat color{};error=suite->PF_GetFloatingPointColorFromColorDef(in->effect_ref,&def,&color);if(!error)std::fprintf(stderr,"FSTR_COLOR_PROBE main=%d stage=%d index=%d value=%.17g,%.17g,%.17g,%.17g raw8=%d,%d,%d\n",pthread_main_np(),stage,i,double(color.red),double(color.green),double(color.blue),double(color.alpha),int(def.u.cd.value.red),int(def.u.cd.value.green),int(def.u.cd.value.blue));auto e=PF_CHECKIN_PARAM(in,&def);if(!error)error=e;}}auto e=in->pica_basicP->ReleaseSuite(kPFColorParamSuite,kPFColorParamSuiteVersion1);return error?error:e?PF_Err_BAD_CALLBACK_PARAM:0;}
PF_Err pre(PF_InData*in,PF_PreRenderExtra*extra){PF_CheckoutResult result{};auto error=extra->cb->checkout_layer(in->effect_ref,0,0,&extra->input->output_request,in->current_time,in->time_step,in->time_scale,&result);if(!error){extra->output->result_rect=result.result_rect;extra->output->max_result_rect=result.max_result_rect;}return error;}
PF_Err render(PF_InData*in,PF_SmartRenderExtra*extra){auto error=values(in);if(error)return error;PF_EffectWorld *source=nullptr,*target=nullptr;error=extra->cb->checkout_layer_pixels(in->effect_ref,0,&source);if(error)return error;error=extra->cb->checkout_output(in->effect_ref,&target);const PF_WorldSuite2*suite=nullptr;bool acquired=false;if(!error){error=in->pica_basicP->AcquireSuite(kPFWorldSuite,kPFWorldSuiteVersion2,reinterpret_cast<const void**>(&suite));acquired=!error;}PF_PixelFormat sf{},tf{};if(!error)error=suite->PF_GetPixelFormat(source,&sf);if(!error)error=suite->PF_GetPixelFormat(target,&tf);std::size_t pixel_size=sf==PF_PixelFormat_ARGB128?sizeof(PF_PixelFloat):sf==PF_PixelFormat_ARGB64?sizeof(PF_Pixel16):sf==PF_PixelFormat_ARGB32?sizeof(PF_Pixel):0;
if(!error&&(!pixel_size||tf!=sf||source->width<1||source->height<1||source->width>16384||source->height>16384||target->width<1||target->height<1||target->width>16384||target->height>16384||!source->data||!target->data||source->rowbytes<source->width*long(pixel_size)||target->rowbytes<target->width*long(pixel_size)))error=PF_Err_BAD_CALLBACK_PARAM;
if(!error)for(A_long y=0;y<target->height;++y){error=PF_ABORT(in);if(error)break;auto dst=reinterpret_cast<char*>(target->data)+std::size_t(y)*std::size_t(target->rowbytes);const long sy=y+target->origin_y-source->origin_y;for(A_long x=0;x<target->width;++x){const long sx=x+target->origin_x-source->origin_x;auto pixel=dst+std::size_t(x)*pixel_size;if(sx<0||sy<0||sx>=source->width||sy>=source->height)std::memset(pixel,0,pixel_size);else{auto row=reinterpret_cast<const char*>(source->data)+std::size_t(sy)*std::size_t(source->rowbytes);std::memcpy(pixel,row+std::size_t(sx)*pixel_size,pixel_size);}}}
auto released=acquired?in->pica_basicP->ReleaseSuite(kPFWorldSuite,kPFWorldSuiteVersion2):0;auto checked=extra->cb->checkin_layer_pixels(in->effect_ref,0);return error?error:released?PF_Err_BAD_CALLBACK_PARAM:checked;}
}
extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr data,PF_PluginDataCB2 callback,SPBasicSuite*,const char*,const char*){PF_Err result=PF_Err_INVALID_CALLBACK;PF_REGISTER_EFFECT_EXT2(data,callback,"FSTR Color Space Probe","FSTR Color Probe 20261018","FSTR.Tools",AE_RESERVED_INFO,"EffectMain","https://github.com/ios3kov/FSTR_LUT");std::fprintf(stderr,"FSTR_COLOR_REGISTER %d\n",int(result));return result;}
extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd,PF_InData*in,PF_OutData*out,PF_ParamDef* parameters[],PF_LayerDef* output,void*extra){try{std::fprintf(stderr,"FSTR_COLOR_COMMAND cmd=%d main=%d ref=%p\n",int(cmd),pthread_main_np(),static_cast<void*>(in->effect_ref));switch(cmd){case PF_Cmd_GLOBAL_SETUP:out->my_version=32769;out->out_flags=PF_OutFlag_DEEP_COLOR_AWARE;out->out_flags2=PF_OutFlag2_SUPPORTS_SMART_RENDER|PF_OutFlag2_FLOAT_COLOR_AWARE;
#ifdef FSTR_COLOR_LIFECYCLE
out->out_flags|=PF_OutFlag_SEND_UPDATE_PARAMS_UI;out->out_flags2|=PF_OutFlag2_SUPPORTS_GET_FLATTENED_SEQUENCE_DATA|PF_OutFlag2_SUPPORTS_THREADED_RENDERING|PF_OutFlag2_SUPPORTS_QUERY_DYNAMIC_FLAGS;
#endif
break;
#ifdef FSTR_COLOR_LIFECYCLE
case PF_Cmd_SEQUENCE_SETUP:case PF_Cmd_SEQUENCE_RESETUP:case PF_Cmd_SEQUENCE_SETDOWN:case PF_Cmd_SEQUENCE_FLATTEN:case PF_Cmd_GET_FLATTENED_SEQUENCE_DATA:return sequence(cmd,in,out);
#endif
case PF_Cmd_PARAMS_SETUP:return params(in,out);case PF_Cmd_SMART_PRE_RENDER:return pre(in,static_cast<PF_PreRenderExtra*>(extra));case PF_Cmd_SMART_RENDER:return render(in,static_cast<PF_SmartRenderExtra*>(extra));case PF_Cmd_RENDER:{auto error=values(in);if(error)return error;PF_InData*in_data=in;return PF_COPY(&parameters[0]->u.ld,output,nullptr,nullptr);}default:break;}return 0;}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_BAD_CALLBACK_PARAM;}}
