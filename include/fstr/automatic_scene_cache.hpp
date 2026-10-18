#pragma once
#include "fstr/scene_cache.hpp"
#include <functional>
namespace fstr {
using SceneCompute=std::function<std::shared_ptr<const SceneAnalysis>(const AnalysisImage&,double,const std::function<void()>&)>;
// Raw current-frame analysis only. Temporal results are derived from explicit
// checked-out inputs, so changing a neighbor cannot reuse an old temporal mask.
class AutomaticSceneCache final {
public:
 AutomaticSceneCache(std::string directory,std::string policy,SceneCompute compute);
 ~AutomaticSceneCache();
 std::shared_ptr<const SceneAnalysis> get(const AnalysisImage&,double,const std::function<void()>& cancellation={},const std::string& source_identity={});
 void clear_memory();
 const std::string& guid()const noexcept;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
