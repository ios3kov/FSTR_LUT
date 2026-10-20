#pragma once
#include <functional>
#include <string>
namespace fstr {
// Verify every installed asset and derive the existing analysis policy without
// constructing inference sessions. Full constructors still check ABI/tensors on
// a miss, and their policy must match before any new observation is accepted.
std::string verified_native_scene_policy(const std::string& resource_directory,
                                        const std::function<void()>& cancel={});
}
