#include "fstr/core.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace fstr {
double srgb_to_linear(double x) {
    return x<=0.04045 ? x/12.92 : std::pow((x+0.055)/1.055,2.4);
}
double linear_to_srgb(double x) {
    return x<=0.0031308 ? x*12.92 : 1.055*std::pow(x,1.0/2.4)-0.055;
}
namespace {
using Clock=std::chrono::steady_clock;
double milliseconds(Clock::time_point start) {
    return std::chrono::duration<double,std::milli>(Clock::now()-start).count();
}
bool cancelled(const RenderContext& ctx) { return ctx.cancelled && ctx.cancelled->load(); }
void validate(FrameView f, const RenderSettings& s, const RenderContext& ctx) {
    if(!f.data || !f.width || !f.height || f.width>4096 || f.height>4096 ||
       f.width>ctx.max_pixels/f.height || f.width>std::numeric_limits<std::size_t>::max()/4 ||
       f.row_stride<f.width*4 || f.row_stride>std::numeric_limits<std::size_t>::max()/f.height ||
       f.count<(f.height-1)*f.row_stride+f.width*4)
        throw std::invalid_argument("Invalid frame bounds/stride/budget");
    for(auto v:{s.enhance,s.look_amount,s.mix})
        if(!std::isfinite(v)||v<0||v>1) throw std::invalid_argument("Strength must be finite [0,1]");
    if(!std::isfinite(s.sigma)||s.sigma<0||s.sigma>75.0f/255.0f)
        throw std::invalid_argument("Invalid model sigma");
}
}
RenderResult render(const FrameWindow& window, const RenderSettings& s, RenderContext& ctx) noexcept {
    RenderResult result;
    auto error_message=[&](const char* text) noexcept {
        try { result.message=text; } catch(...) { result.message.clear(); }
    };
    try {
        std::lock_guard<std::mutex> lock(ctx.mutex);
        auto f=window.current; validate(f,s,ctx);
        result.width=f.width; result.height=f.height;
        if(cancelled(ctx)) { result.status=Status::Cancelled; result.message="Cancelled"; return result; }
        if(window.temporal_radius!=0 || f.memory!=Memory::CPU) {
            result.status=Status::Unsupported; result.message="Prototype requires radius-zero CPU frame"; return result;
        }
        result.rgba.resize(f.width*f.height*4);
        for(std::size_t y=0;y<f.height;++y) {
            if(cancelled(ctx)) {result.rgba.clear();result.status=Status::Cancelled;return result;}
            std::copy_n(f.data+y*f.row_stride,f.width*4,result.rgba.data()+y*f.width*4);
        }
        if(s.mix==0 || (s.enhance==0 && (!s.look || s.look_amount==0))) {
            result.status=Status::Ok; return result;
        }
        if(f.primaries!=Primaries::SRGB_Rec709 ||
           (f.transfer!=Transfer::SRGB && f.transfer!=Transfer::Linear) ||
           (f.alpha!=Alpha::Straight && f.alpha!=Alpha::Premultiplied)) {
            result.status=Status::Unsupported; result.message="Explicit sRGB/linear-sRGB contract required";
            result.rgba.clear(); return result;
        }
        auto prep=Clock::now();
        std::vector<float> encoded(result.rgba.size());
        for(std::size_t p=0;p<f.width*f.height;++p) {
            const auto alpha=result.rgba[p*4+3];
            if(!std::isfinite(alpha)||alpha<0||alpha>1) throw std::invalid_argument("Invalid alpha");
            if(s.enhance>0 && alpha!=1) {
                result.status=Status::Unsupported;result.message="AI transparency not supported in research slice";
                result.rgba.clear();return result;
            }
            for(std::size_t c=0;c<3;++c) {
                double value=result.rgba[p*4+c];
                if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite input");
                if(f.alpha==Alpha::Premultiplied && alpha>0) value/=alpha;
                if(f.transfer==Transfer::Linear) value=linear_to_srgb(value);
                if(!std::isfinite(value) || std::abs(value)>std::numeric_limits<float>::max())
                    throw std::invalid_argument("Transfer exceeds float representability");
                if(s.enhance>0 && (value<0||value>1)) {
                    result.status=Status::Unsupported;result.message="AI expects opaque SDR [0,1]; input not clipped";
                    result.rgba.clear();return result;
                }
                encoded[p*4+c]=static_cast<float>(value);
            }
            encoded[p*4+3]=alpha;
        }
        result.preparation_ms=milliseconds(prep);
        if(s.enhance>0) {
            if(!ctx.backend) throw std::runtime_error("No model backend");
            if((f.width+1)/2 > 524288/((f.height+1)/2)) {
                result.status=Status::Unsupported;result.message="Padded frame exceeds research inference budget";
                result.rgba.clear();return result;
            }
            auto id=ctx.backend->identity();
            if(ctx.cached_source.capacity()+ctx.cached_restored.capacity()>ctx.cache_budget_bytes/sizeof(float)) {
                std::vector<float>().swap(ctx.cached_source);std::vector<float>().swap(ctx.cached_restored);
                ctx.cached_width=ctx.cached_height=0;ctx.cached_backend.clear();
            }
            // One bounded exact-content cache; padding bytes and look settings excluded.
            result.restoration_cache_hit=ctx.cached_width==f.width && ctx.cached_height==f.height &&
                ctx.cached_sigma==s.sigma && ctx.cached_backend==id && ctx.cached_source==encoded &&
                ctx.cached_restored.size()==encoded.size();
            if(result.restoration_cache_hit) ctx.restored=ctx.cached_restored;
            else {
                auto t=Clock::now();
                pack_ffdnet(encoded,f.width,f.height,s.sigma,ctx.packed);
                ctx.backend->infer(ctx.packed,ctx.planes,ctx.cancelled);
                if(cancelled(ctx)) {result.status=Status::Cancelled;result.rgba.clear();return result;}
                unpack_ffdnet(ctx.planes,f.width,f.height,ctx.restored);
                result.inference_ms=milliseconds(t);
                if(encoded.size()<=ctx.cache_budget_bytes/(2*sizeof(float))) {
                    std::vector<float> source_snapshot(encoded),restored_snapshot(ctx.restored);
                    ctx.cached_width=ctx.cached_height=0;ctx.cached_backend.clear();ctx.cached_sigma=-1;
                    if(source_snapshot.capacity()+restored_snapshot.capacity()<=ctx.cache_budget_bytes/sizeof(float)) {
                        ctx.cached_source.swap(source_snapshot);ctx.cached_restored.swap(restored_snapshot);
                        ctx.cached_sigma=s.sigma;ctx.cached_backend=id;
                        ctx.cached_width=f.width;ctx.cached_height=f.height;
                    } else {
                        std::vector<float>().swap(ctx.cached_source);std::vector<float>().swap(ctx.cached_restored);
                    }
                } else {
                    std::vector<float>().swap(ctx.cached_source);std::vector<float>().swap(ctx.cached_restored);
                    ctx.cached_width=ctx.cached_height=0;ctx.cached_backend.clear();
                }
            }
        }
        auto finish=Clock::now();
        for(std::size_t p=0;p<f.width*f.height;++p) {
            if((p%4096)==0 && cancelled(ctx)) {result.status=Status::Cancelled;result.rgba.clear();return result;}
            double alpha=result.rgba[p*4+3];
            if(alpha==0) continue; // Preserve hidden RGB, never divide by zero.
            std::array<double,3> original{},graded{};
            for(std::size_t c=0;c<3;++c) {
                original[c]=srgb_to_linear(encoded[p*4+c]);
                auto restored=s.enhance>0 ? srgb_to_linear(ctx.restored[p*4+c]) : original[c];
                graded[c]=original[c]+s.enhance*(restored-original[c]);
            }
            if(s.look && s.look_amount>0) {
                auto mapped=s.look->sample({linear_to_srgb(graded[0]),linear_to_srgb(graded[1]),linear_to_srgb(graded[2])});
                auto values=std::array<double,3>{mapped.r,mapped.g,mapped.b};
                for(std::size_t c=0;c<3;++c) graded[c]+=s.look_amount*(srgb_to_linear(values[c])-graded[c]);
            }
            for(std::size_t c=0;c<3;++c) {
                auto value=original[c]+s.mix*(graded[c]-original[c]);
                if(f.transfer==Transfer::SRGB) value=linear_to_srgb(value);
                if(f.alpha==Alpha::Premultiplied) value*=alpha;
                if(!std::isfinite(value)||std::abs(value)>std::numeric_limits<float>::max())
                    throw std::invalid_argument("Processing exceeds float representability");
                result.rgba[p*4+c]=static_cast<float>(value);
            }
        }
        result.look_mix_ms=milliseconds(finish); result.status=Status::Ok; return result;
    } catch(const std::invalid_argument& error) {result.status=Status::InvalidInput;error_message(error.what());}
      catch(const std::exception& error) {result.status=cancelled(ctx)?Status::Cancelled:Status::BackendError;error_message(error.what());}
      catch(...) {result.status=Status::BackendError;error_message("Unknown processing error");}
    result.rgba.clear(); return result;
}
} // namespace fstr
