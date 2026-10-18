#pragma once
#include "plugin.hpp"
#include "AE_EffectSuites.h"
#include "fstr/sdr_timeline.hpp"
#include "fstr/sdr_prepare_cache.hpp"
#include "fstr/regions.hpp"
namespace fstr::ae {
struct HostSdrTimeline {
    std::vector<SdrShotRange> shots;
    std::vector<CachedSdrPreparation> recipes;
    const CachedSdrPreparation& at(std::size_t frame)const;
    const SdrShotRange& range(std::size_t frame)const;
};
PF_State sdr_input_state(PF_InData*);
std::shared_ptr<const HostSdrTimeline> get_sdr_timeline(PF_InData*,const PF_State&,const std::string&,
    const std::function<std::shared_ptr<const HostSdrTimeline>()>&);
void clear_sdr_timelines();
struct PreparedInput {
    AnalysisImage image;
    std::string identity;
    std::size_t cache_bytes()const noexcept{return sizeof(PreparedInput)+image.rgba.capacity()+identity.capacity();}
};
std::shared_ptr<const PreparedInput> get_prepared_input(PF_InData*,const PF_State&,const std::string&,
    const std::function<PreparedInput()>&);
void clear_prepared_inputs();
}
