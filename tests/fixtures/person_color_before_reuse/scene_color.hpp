// Frozen 5f2ae8c numerical/error oracle; namespace-only isolation.
#pragma once
#include "fstr/color.hpp"
#include "skin_tone.hpp"
#include <array>
namespace fstr::frozen {
// Semantic probabilities after confidence/spatial/temporal processing; omitted
// mass belongs to unknown and receives exactly the common look.
enum class SceneRole : std::size_t { Skin, Hair, Clothing, Sky, Vegetation, Water, Built, Emissive, Count };
constexpr std::size_t scene_role_count=static_cast<std::size_t>(SceneRole::Count);
using SceneWeights=std::array<float,scene_role_count>;
void validate_scene_weights(const SceneWeights&);
// Experimental Teal & Orange v1 residual. Straight linear Rec709 only, after
// explicit source normalization. Not an illumination estimator or a skin target.
struct StableToneWeight { double support=0,strength=0; };
class SceneLook final {
public:
    SceneLook(std::shared_ptr<const LUT> teal_orange,double strength);
    std::array<float,4> apply_people(std::array<float,4>,const SceneWeights&,const std::vector<SkinToneInfluence>&,bool enabled=true) const;
    std::array<float,4> apply_stable_people(std::array<float,4>,const SceneWeights&,const std::vector<SkinToneInfluence>&,const std::vector<StableToneWeight>&,bool enabled=true) const;
    // Reuses the same float-rounded ordinary result for every semantic/person
    // branch and for this double-precision difference. No external graded input.
    std::array<double,3> stable_residual(std::array<float,4>,const SceneWeights&,const std::vector<SkinToneInfluence>&,const std::vector<StableToneWeight>&,bool enabled=true) const;
    SkinToneReference skin_reference(const std::vector<SkinSample>& samples)const{return skin_.reference(samples);}
    std::array<float,4> apply(std::array<float,4>,const SceneWeights&,bool enabled=true,const SkinToneReference* person=nullptr) const;
private:
    std::array<float,4> apply_from_ordinary(std::array<float,4>,std::array<float,4>,const SceneWeights&,bool,const SkinToneReference*) const;
    std::array<float,4> people_from_ordinary(std::array<float,4>,std::array<float,4>,const SceneWeights&,const std::vector<SkinToneInfluence>&,bool) const;
    std::array<float,4> stable_from_ordinary(std::array<float,4>,std::array<float,4>,const SceneWeights&,const std::vector<SkinToneInfluence>&,const std::vector<StableToneWeight>&,bool) const;
    ColorProcessor base_;
    SkinToneLook skin_;
    double strength_;
};
}
