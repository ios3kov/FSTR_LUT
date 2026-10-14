#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace fstr {
enum class Transfer { SRGB, Linear };
enum class Primaries { SRGB_Rec709, Unknown };
enum class Alpha { Straight, Premultiplied };
enum class Memory { CPU, GPU };
// Borrowed immutable storage; row_stride/count are in floats. Lifetime covers render.
struct FrameView {
    const float* data = nullptr;
    std::size_t count = 0, width = 0, height = 0, row_stride = 0;
    Transfer transfer = Transfer::SRGB;
    Primaries primaries = Primaries::SRGB_Rec709;
    Alpha alpha = Alpha::Straight;
    Memory memory = Memory::CPU;
};
struct FrameWindow { FrameView current; int temporal_radius = 0; };
struct RGB { double r, g, b; };
class LUT final {
public:
    static LUT parse(std::istream& stream);
    static LUT identity(std::size_t size);
    RGB sample(RGB input) const;
    std::size_t size() const { return size_; }
private:
    std::size_t size_ = 0;
    RGB minimum_{0,0,0}, maximum_{1,1,1};
    std::vector<RGB> nodes_;
};
struct PackedTensor {
    std::size_t half_width = 0, half_height = 0;
    std::vector<float> data;
};
void pack_ffdnet(const std::vector<float>& rgba, std::size_t width,
                 std::size_t height, float sigma, PackedTensor& destination);
void unpack_ffdnet(const std::vector<float>& planes, std::size_t width,
                   std::size_t height, std::vector<float>& destination);
double srgb_to_linear(double value);
double linear_to_srgb(double value);

class InferenceBackend {
public:
    virtual ~InferenceBackend() = default;
    virtual std::string identity() const = 0;
    // Output is twelve NCHW planes, dimensions equal packed half dimensions.
    virtual void infer(const PackedTensor& input, std::vector<float>& output,
                       const std::atomic_bool* cancelled) = 0;
};
struct RenderSettings {
    double enhance = 0.5, look_amount = 1.0, mix = 1.0;
    float sigma = 15.0f/255.0f; // Experimental model input, not product auto-estimation.
    std::shared_ptr<const LUT> look;
};
enum class Status { Ok, InvalidInput, Unsupported, Cancelled, BackendError };
// Caller owns context, render serializes simultaneous use; backend may be shared.
struct RenderContext {
    std::shared_ptr<InferenceBackend> backend;
    const std::atomic_bool* cancelled = nullptr;
    std::size_t max_pixels = 2097152, cache_budget_bytes = 64u*1024u*1024u;
    std::mutex mutex;
    PackedTensor packed;
    std::vector<float> planes, restored, cached_source, cached_restored;
    std::size_t cached_width = 0, cached_height = 0;
    float cached_sigma = -1;
    std::string cached_backend;
};
struct RenderResult {
    Status status = Status::BackendError;
    std::string message;
    std::size_t width = 0, height = 0;
    std::vector<float> rgba;
    bool restoration_cache_hit = false;
    double preparation_ms = 0, inference_ms = 0, look_mix_ms = 0;
};
RenderResult render(const FrameWindow&, const RenderSettings&, RenderContext&) noexcept;
} // namespace fstr
