#include "fstr/color.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
namespace fstr {
namespace {
bool cancelled(const ColorContext& c) {return c.cancelled&&c.cancelled->load();}
void validate(FrameView f,const ColorContext& c) {
    const auto limit=std::numeric_limits<std::size_t>::max();
    if(!f.data||!f.width||!f.height||f.width>16384||f.height>16384||
       f.width>c.max_pixels/f.height||f.width>limit/4||f.row_stride<f.width*4||
       f.row_stride>limit/f.height||f.count<(f.height-1)*f.row_stride+f.width*4)
        throw std::invalid_argument("Invalid color frame bounds/stride/budget");
}
}
RenderResult render_color(const FrameWindow& w,const ColorSettings& s,const ColorContext& c) noexcept {
    RenderResult r;
    auto message=[&](const char* m) noexcept {try{r.message=m;}catch(...){r.message.clear();}};
    try {
        const auto f=w.current;validate(f,c);r.width=f.width;r.height=f.height;
        if(cancelled(c)){r.status=Status::Cancelled;return r;}
        if(f.memory!=Memory::CPU||w.temporal_radius!=0){r.status=Status::Unsupported;message("Color slice requires radius-zero CPU frame");return r;}
        const ColorProcessor processor(s,f.transfer,f.primaries,f.alpha);
        r.rgba.resize(f.width*f.height*4);
        for(std::size_t y=0;y<f.height;++y) {
            if(cancelled(c)){r.status=Status::Cancelled;r.rgba.clear();return r;}
            if(processor.bypass()) {
                // Exact object bytes, including signed zero and NaN payloads; omit padding.
                std::memcpy(r.rgba.data()+y*f.width*4,f.data+y*f.row_stride,f.width*4*sizeof(float));
                continue;
            }
            for(std::size_t x=0;x<f.width;++x) {
                if(x%4096==0&&cancelled(c)){r.status=Status::Cancelled;r.rgba.clear();return r;}
                const auto input=f.data+y*f.row_stride+x*4;auto output=r.rgba.data()+(y*f.width+x)*4;
                const auto pixel=processor.apply({input[0],input[1],input[2],input[3]});
                std::copy(pixel.begin(),pixel.end(),output);
            }
        }
        r.status=Status::Ok;return r;
    }catch(const UnsupportedColorProfile& e){r.status=Status::Unsupported;message(e.what());}
     catch(const std::invalid_argument& e){r.status=Status::InvalidInput;message(e.what());}
     catch(const std::exception& e){r.status=Status::BackendError;message(e.what());}
     catch(...){r.status=Status::BackendError;message("Unknown color processing error");}
    r.rgba.clear();return r;
}
} // namespace fstr
