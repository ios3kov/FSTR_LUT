#include "AEConfig.h"
#include "AE_EffectVers.h"
#include <AE_General.r>
resource 'PiPL' (16000) {
    { Kind { AEEffect }, Name { "FSTR-LUT Development" }, Category { "FSTR.Tools" },
      CodeMacARM64 { "EffectMain" },
      AE_PiPL_Version {2,0}, AE_Effect_Spec_Version {PF_PLUG_IN_VERSION,PF_PLUG_IN_SUBVERS},
      AE_Effect_Version {32769}, AE_Effect_Info_Flags {0},
      AE_Effect_Global_OutFlags {0x02008042}, AE_Effect_Global_OutFlags_2 {0x08221401},
      AE_Effect_Match_Name {"FSTR LUT Color MVP"}, AE_Reserved_Info {0},
      AE_Effect_Support_URL {"https://github.com/ios3kov/FSTR_LUT"}
    }
};
