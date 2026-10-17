#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace fstr {
struct AnalysisImage {int width=0,height=0;std::vector<std::uint8_t> rgba;};
struct ColorRegion {double x0=0,y0=0,x1=0,y1=0,detail=0;std::vector<std::vector<std::array<double,2>>> holes;};
void prepare_region_cache();
std::vector<ColorRegion> detect_color_regions(const AnalysisImage&, double source_height);
// Coordinates normalized to the same complete layer frame, never output ROI.
std::vector<ColorRegion> stabilize_color_regions(const std::vector<ColorRegion>& current,
    const std::vector<ColorRegion>& before,const std::vector<ColorRegion>& after);
struct RegionFrame {int offset=0;AnalysisImage image;std::vector<ColorRegion> regions;};
// Complete explicit window, offsets -2..2. Only raw detections are seeds.
std::vector<ColorRegion> recover_color_regions(const std::vector<RegionFrame>&);
double color_region_weight(double x,double y,double source_height,const std::vector<ColorRegion>&);
}
