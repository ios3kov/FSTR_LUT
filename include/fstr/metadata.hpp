#pragma once
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

namespace fstr {
enum class MetadataStatus { Found, NotFound, Unsupported, Invalid, Ambiguous, LimitExceeded, IoError };
enum class DeclaredProfile { Unknown, Rec709, SonySLog3Cine, Other };
struct CameraMetadata {
    MetadataStatus status=MetadataStatus::NotFound;
    DeclaredProfile profile=DeclaredProfile::Unknown;
    std::string manufacturer,model,gamma,primaries,coding;
    std::string provenance; // Recognized source schema; no private identifiers.
    std::string message;
    std::size_t bytes_read=0;
};
struct MetadataLimits {
    std::size_t read_bytes=4*1024*1024;
    std::size_t xml_bytes=256*1024;
    std::size_t boxes=4096;
};
// Source declarations only. Never selects the effect's incoming pixel encoding.
CameraMetadata parse_sony_metadata_xml(std::string_view xml,MetadataLimits limits={});
CameraMetadata parse_xmp_metadata(std::string_view xml,MetadataLimits limits={});
// Bounded seek/read of small metadata boxes; skips compressed frame payloads.
// Caller must keep filesystem work outside frame rendering.
CameraMetadata read_camera_metadata(const std::filesystem::path& path,MetadataLimits limits={});
const char* metadata_status_name(MetadataStatus) noexcept;
const char* declared_profile_name(DeclaredProfile) noexcept;
}
