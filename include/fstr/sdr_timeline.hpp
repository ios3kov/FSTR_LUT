#pragma once
#include "fstr/sdr_prepare.hpp"
#include <array>
namespace fstr {
inline constexpr std::size_t sdr_timeline_frame_budget=16384;
struct SdrCutFeature {
    std::array<float,96> tiles{}; // linear Rec709 RGB, 8x4 cells
    std::array<float,48> histogram{}; // normalized encoded RGB, 16 bins/channel
    float coverage=0;
};
struct SdrShotRange {
    std::size_t begin=0,end=0; // frame indices, half-open
    std::array<std::size_t,3> anchors{};
    bool ambiguous=false; // sustained gradual/complex change: neutral preparation
};
SdrCutFeature describe_sdr_cut_frame(FrameView,const ColorContext& = {});
// Complete ordered per-frame descriptors. Never a sparse sample or previousFrame.
// One-frame excursions that return to the preceding image are excluded from
// anchor selection, rather than changing the shot's grade on the flash frame.
std::vector<SdrShotRange> find_sdr_shots(const std::vector<SdrCutFeature>&,const ColorContext& = {});
}
