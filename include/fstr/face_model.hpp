#pragma once
#include "fstr/regions.hpp"
#include <functional>
#include <memory>
#include <string>
namespace fstr {
// Boxes are analysis-image pixels; always raw current-frame observations.
struct FaceProposal {std::array<double,4> box{};double score=0;int scale_agreement=0;};
class FaceModel final {
public:
 explicit FaceModel(const std::string& model_directory);
 ~FaceModel();
 std::vector<FaceProposal> analyze(const AnalysisImage&,const std::function<void()>& cancellation={});
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
