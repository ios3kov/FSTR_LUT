#pragma once
#include "fstr/scene_cache.hpp"
namespace fstr {
struct AnalyzedFrame {int offset=0;std::shared_ptr<const SceneAnalysis> analysis;};
// Only scalar confidence and reference samples cross frames. Geometry always
// belongs to the current frame. Ambiguous identities are never forcibly matched.
std::shared_ptr<const SceneAnalysis> stabilize_scene_window(const std::vector<AnalyzedFrame>&);
}
