#pragma once
#define PF_DEEP_COLOR_AWARE 1
#include "AEConfig.h"
#include "SPBasic.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "fstr/color.hpp"
enum Param { Input=0, Exposure, Temperature, Tint, Contrast, Saturation, Shoulder, Look, LookAmount, Mix, AdvancedStart, Source, WorkingSpace, NaturalContrast, AdvancedEnd, SmartColor, CameraInfo, AutoPrepare, ImportLUT, LUTReference, LUTFileInfo, ParamCount };
constexpr int RenderParamCount=ParamCount-6; // Exclude input, topics, metadata, button and filename display.
// Persistent IDs retain the first native prototype's meaning despite UI order changes.
enum DiskId { SourceId=1, ExposureId=2, ContrastId=3, SaturationId=4, ShoulderId=5, LookId=6, LookAmountId=7, MixId=8, WorkingSpaceId=9, TemperatureId=10, TintId=11, CameraInfoId=12, SmartColorId=13, AdvancedStartId=14, AdvancedEndId=15, NaturalContrastId=16, AutoPrepareId=17, ImportLUTId=18, ExternalLUTId=19, LUTFileInfoId=20 };
extern "C" DllExport PF_Err EffectMain(PF_Cmd,PF_InData*,PF_OutData*,PF_ParamDef*[],PF_LayerDef*,void*);
