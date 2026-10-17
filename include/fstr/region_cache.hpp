#pragma once
#include "fstr/regions.hpp"
#include <functional>
#include <memory>
#include <string>
namespace fstr {
// Stores analysis only: look, strength and Smart switch are deliberately absent.
class RegionCache final {
public:
 explicit RegionCache(std::string directory={},std::size_t entries=64);
 ~RegionCache();
 std::vector<ColorRegion> get(const AnalysisImage&,double source_height,
     const std::string& algorithm,const std::function<std::vector<ColorRegion>()>& compute);
 void clear_memory();
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
