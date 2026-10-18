#pragma once
#include "fstr/regions.hpp"
#include "fstr/scene_color.hpp"
#include <memory>
#include <string>
namespace fstr {
struct PersonAnalysis {std::array<double,4> box{};std::array<double,3> descriptor{};double detection_confidence=0;std::vector<float> mask,toning_support;double toning_strength=0;std::vector<SkinSample> samples;};
struct SceneAnalysis {int width=0,height=0;std::vector<SceneWeights> scene;std::vector<PersonAnalysis> people;};
// Read-only development bridge to externally computed analysis. It does not run models.
// Ready records are immutable; directory/policy configured before rendering.
std::string scene_cache_key(const AnalysisImage&,double source_height,const std::string& policy);
class SceneCache final {
public:
 SceneCache(std::string directory,std::string policy);
 ~SceneCache();
 std::shared_ptr<const SceneAnalysis> get(const AnalysisImage&,double source_height);
 void clear_memory();
 const std::string& guid() const noexcept;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
