#include "fstr/core.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace fstr {
namespace {
std::size_t checked_pixels(std::size_t w, std::size_t h) {
    if (!w || !h || w > 4096 || h > 4096 || w > 2097152/h)
        throw std::invalid_argument("Model dimensions exceed prototype limit");
    if ((w+1)/2 > 524288/((h+1)/2))
        throw std::invalid_argument("Padded model dimensions exceed prototype limit");
    return w*h;
}
}
void pack_ffdnet(const std::vector<float>& rgba, std::size_t w, std::size_t h,
                 float sigma, PackedTensor& dst) {
    auto pixels = checked_pixels(w,h);
    if (rgba.size()!=pixels*4 || !std::isfinite(sigma) || sigma<0 || sigma>75.0f/255.0f)
        throw std::invalid_argument("Invalid RGB tensor or sigma");
    dst.half_width=(w+1)/2; dst.half_height=(h+1)/2;
    const auto plane=dst.half_width*dst.half_height;
    dst.data.resize(13*plane);
    for (std::size_t c=0;c<3;++c)
        for (std::size_t dy=0;dy<2;++dy)
            for (std::size_t dx=0;dx<2;++dx)
                for (std::size_t y=0;y<dst.half_height;++y)
                    for (std::size_t x=0;x<dst.half_width;++x) {
                        auto sy=std::min(2*y+dy,h-1), sx=std::min(2*x+dx,w-1);
                        auto value=rgba[(sy*w+sx)*4+c];
                        if (!std::isfinite(value) || value<0 || value>1)
                            throw std::invalid_argument("Model input must be finite encoded SDR [0,1]");
                        dst.data[(c*4+dy*2+dx)*plane+y*dst.half_width+x]=value;
                    }
    std::fill(dst.data.begin()+static_cast<std::ptrdiff_t>(12*plane),dst.data.end(),sigma);
}
void unpack_ffdnet(const std::vector<float>& planes, std::size_t w, std::size_t h,
                   std::vector<float>& dst) {
    auto pixels=checked_pixels(w,h);
    auto hw=(w+1)/2, hh=(h+1)/2, plane=hw*hh;
    if (planes.size()!=12*plane) throw std::invalid_argument("Unexpected model output shape");
    dst.resize(pixels*4);
    for (std::size_t y=0;y<h;++y)
        for (std::size_t x=0;x<w;++x) {
            for (std::size_t c=0;c<3;++c) {
                auto value=planes[(c*4+(y%2)*2+x%2)*plane+(y/2)*hw+x/2];
                if (!std::isfinite(value)) throw std::invalid_argument("Nonfinite model output");
                dst[(y*w+x)*4+c]=value;
            }
            dst[(y*w+x)*4+3]=1;
        }
}
} // namespace fstr
