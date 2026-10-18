#pragma once
#include "plugin.hpp"
#include "AE_EffectUI.h"
namespace fstr::ae {
class ExternalLUTError final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
struct ExternalLookValue {std::shared_ptr<const LUT> look;std::string path,identity,error;};
PF_ArbitraryH new_external_reference(PF_InData*);
ExternalLookValue external_reference(PF_InData*,PF_ArbitraryH);
PF_Err external_arbitrary(PF_InData*,PF_ArbParamsExtra*);
PF_Err import_external_lut(PF_InData*,PF_OutData*,PF_ParamDef*[]);
std::string display_lut_name(const std::string&);
std::string choose_lut_file(const std::string& current);
void clear_external_references();
PF_Err refresh_external_menu(PF_InData*,PF_ParamDef*[]);
void remember_external_selection(PF_InData*,int);
A_short previous_external_selection(PF_InData*);
void forget_external_selection(PF_InData*);
PF_Err draw_info_line(PF_InData*,PF_EventExtra*,const std::string&);
}
