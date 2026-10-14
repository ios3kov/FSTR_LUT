#pragma once
#include "fstr/core.hpp"
#include <filesystem>
namespace fstr {
std::shared_ptr<InferenceBackend> load_ort_cpu(const std::filesystem::path& manifest);
}
