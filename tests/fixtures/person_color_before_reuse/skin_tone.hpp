// Frozen 5f2ae8c numerical/error oracle; namespace-only isolation.
#pragma once
#include "fstr/color.hpp"
#include <vector>
namespace fstr::frozen {
// Explicit samples belonging to one person in one shot, straight linear Rec709.
// Association and semantic confidence are supplied by analysis, never guessed here.
struct SkinSample { RGB color; double confidence=0; };
struct SkinToneReference {
    RGB chroma_delta{}; // LUT-directed change in RGB / luminance, zero luminance.
    double confidence=0;
    std::size_t samples=0;
private:
    friend class SkinToneLook;
    std::shared_ptr<const int> binding_; // In-process immutable look instance; never serialize.
};
struct SkinToneInfluence { const SkinToneReference* reference=nullptr; double weight=0; };
class SkinToneLook final {
public:
    SkinToneLook(std::shared_ptr<const LUT>,double strength);
    std::array<float,4> apply_people(std::array<float,4>,const std::vector<SkinToneInfluence>&,bool enabled=true) const;
    SkinToneReference reference(const std::vector<SkinSample>&) const;
    std::array<float,4> apply(std::array<float,4>,const SkinToneReference&,double mask,bool enabled=true) const;
private:
    friend class SceneLook;
    // The caller is this immutable look or its owning SceneLook, constructed
    // from the identical LUT/strength/linear-straight contract.
    std::array<float,4> apply_from_ordinary(std::array<float,4>,std::array<float,4>,const SkinToneReference&,double,bool) const;
    std::array<float,4> people_from_ordinary(std::array<float,4>,std::array<float,4>,const std::vector<SkinToneInfluence>&,bool) const;
    ColorProcessor ordinary_;
    std::shared_ptr<const int> binding_=std::make_shared<const int>(0);
};
}
