#pragma once
#include "fstr/parts_model.hpp"
#include "fstr/scene_color.hpp"
namespace fstr {
struct SceneModelResult {int width=0,height=0;std::vector<SceneWeights> weights;std::vector<float> person;};
class SceneModel final {
public:
 explicit SceneModel(const std::string& model_directory);
 ~SceneModel();
 const std::string& runtime_identity()const noexcept;
 SceneModelResult analyze(const AnalysisImage&,const PersonParts&,const std::function<void()>& cancellation={});
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
