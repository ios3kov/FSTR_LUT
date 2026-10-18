#pragma once
#include "fstr/sdr_prepare.hpp"
#include <functional>
namespace fstr {
enum class SdrCacheUse { Computed, Memory, Disk };
struct CachedSdrPreparation {SdrShotPreparation shot;std::string key;SdrCacheUse use=SdrCacheUse::Computed;};
// Small96x54 CPU snapshots only. No host pointers survive get(). Metadata scopes
// anchor times/full geometry. Pixel content is hashed here, not trusted to caller.
class SdrPreparationCache final {
public:
    explicit SdrPreparationCache(std::string directory={});
    ~SdrPreparationCache();
    CachedSdrPreparation get(const std::vector<FrameView>&,const std::string& metadata,
                             const std::function<void()>& cancel={});
    void clear_memory();
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
std::vector<std::int32_t> sdr_clip_anchor_times(std::int32_t total,std::int32_t step);
}
