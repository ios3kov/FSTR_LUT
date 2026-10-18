#pragma once
#include "fstr/core.hpp"
namespace fstr {
// File I/O and processor creation happen only at import/project restoration.
// Project persistence stores the external reference, never LUT source/weights.
struct ExternalLUT {std::shared_ptr<const LUT> look;std::string identity;std::size_t bytes=0;};
ExternalLUT load_external_lut(const std::string& absolute_path);
}
