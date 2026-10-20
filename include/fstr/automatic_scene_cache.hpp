#pragma once
#include "fstr/scene_cache.hpp"
#include <functional>
#include <cstdint>
namespace fstr {
using SceneCompute=std::function<std::shared_ptr<const SceneAnalysis>(const AnalysisImage&,double,const std::function<void()>&)>;
// Raw current-frame analysis only. Temporal results are derived from explicit
// checked-out inputs, so changing a neighbor cannot reuse an old temporal mask.
struct SceneDiskBudget {
 std::uint64_t bytes=8ULL*1024*1024*1024;
 std::uint64_t free_reserve=2ULL*1024*1024*1024;
};
class AutomaticSceneCache final {
public:
 // Two computes require a concurrently safe callback; native model stages
 // each serialize access to their own model.
 // No workers are created; callers retain image/context lifetime until return.
 AutomaticSceneCache(std::string directory,std::string policy,SceneCompute compute,SceneDiskBudget budget={},unsigned max_computes=1);
 ~AutomaticSceneCache();
 std::shared_ptr<const SceneAnalysis> get(const AnalysisImage&,double,const std::function<void()>& cancellation={},const std::string& source_identity={});
 // Clears resident entries immediately; older requests cannot republish into
 // the new generation. Persistent entries remain available on the next get.
 void clear_memory();
 const std::string& guid()const noexcept;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
