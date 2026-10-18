#pragma once
#include "fstr/scene_cache.hpp"
#include <functional>
namespace fstr {
// All assets validated/loaded by construction outside host render callbacks.
// Current-frame, IMAGE-mode observations only. No implicit temporal state.
class NativeSceneAnalyzer final {
public:
 explicit NativeSceneAnalyzer(const std::string& resource_directory);
 ~NativeSceneAnalyzer();
 std::shared_ptr<const SceneAnalysis> analyze(const AnalysisImage&,double source_height,const std::function<void()>& cancellation={});
 const std::string& policy() const noexcept;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
