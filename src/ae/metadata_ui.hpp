#pragma once
#include "plugin.hpp"
#include "AE_EffectUI.h"
namespace fstr::ae {
PF_Err metadata_event(PF_InData*,PF_EventExtra*);
void clear_metadata_cache(PF_ProgPtr);
void clear_all_metadata_cache();
}
