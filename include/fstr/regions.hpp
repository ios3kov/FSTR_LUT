#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace fstr {
struct AnalysisImage {int width=0,height=0;std::vector<std::uint8_t> rgba;};
struct ColorRegion {double x0=0,y0=0,x1=0,y1=0,detail=0;std::vector<std::vector<std::array<double,2>>> holes;};
std::vector<ColorRegion> detect_color_regions(const AnalysisImage&, double source_height);
// Coordinates normalized to the same complete layer frame, never output ROI.
std::vector<ColorRegion> stabilize_color_regions(const std::vector<ColorRegion>& current,
    const std::vector<ColorRegion>& before,const std::vector<ColorRegion>& after);
double color_region_weight(double x,double y,double source_height,const std::vector<ColorRegion>&);
}
