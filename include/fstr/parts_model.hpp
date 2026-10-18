#pragma once
#include "fstr/regions.hpp"
#include <array>
#include <functional>
#include <memory>
#include <string>
namespace fstr {
// MediaPipe selfie-multiclass v1; interleaved confidence, CPU-owned storage.
// Labels: background, hair, body-skin, face-skin, clothes, others.
struct PersonParts {int width=0,height=0;std::vector<std::array<float,6>> probabilities;};
// Construct outside host render callbacks. No Python/interpreter, no video state.
class PartsModel final {
public:
 PartsModel(const std::string& library,const std::string& library_sha256,const std::string& model,const std::string& face_model={},const std::string& pose_model={});
 ~PartsModel();
 // Face crop -> normalized478point mesh; empty means no reliable detection.
 std::vector<std::array<float,3>> face_mesh(const AnalysisImage&,const std::function<void()>& cancellation={});
 std::vector<std::vector<float>> people(const AnalysisImage&,const std::function<void()>& cancellation={});
 PersonParts analyze(const AnalysisImage&,const std::function<void()>& cancellation={});
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
