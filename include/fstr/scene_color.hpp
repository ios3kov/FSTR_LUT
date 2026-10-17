#pragma once
#include "fstr/color.hpp"
#include "fstr/skin_tone.hpp"
#include <array>
namespace fstr {
// Semantic probabilities after confidence/spatial/temporal processing; omitted
// mass belongs to unknown and receives exactly the common look.
enum class SceneRole : std::size_t { Skin, Hair, Clothing, Sky, Vegetation, Water, Built, Emissive, Count };
constexpr std::size_t scene_role_count=static_cast<std::size_t>(SceneRole::Count);
using SceneWeights=std::array<float,scene_role_count>;
void validate_scene_weights(const SceneWeights&);
// Experimental Teal & Orange v1 residual. Straight linear Rec709 only, after
// explicit source normalization. Not an illumination estimator or a skin target.
class SceneLook final {
public:
    SceneLook(std::shared_ptr<const LUT> teal_orange,double strength);
    SkinToneReference skin_reference(const std::vector<SkinSample>& samples)const{return skin_.reference(samples);}
    std::array<float,4> apply(std::array<float,4>,const SceneWeights&,bool enabled=true,const SkinToneReference* person=nullptr) const;
private:
    ColorProcessor base_;
    SkinToneLook skin_;
    double strength_;
};
}
