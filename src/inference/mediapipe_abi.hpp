#pragma once
#include <cstddef>
#include <cstdint>
// Narrow ABI adapter for MediaPipe 1.1.1's installed C bindings. Layout and
// signatures checked against the shipped Python ctypes declarations; see
// docs/NATIVE_MODEL_PORT.md. No Python API is called or linked.
namespace fstr::mpabi {
struct BaseOptions {const char* model_asset_buffer=nullptr;unsigned model_asset_buffer_count=0;const char* model_asset_path=nullptr;int file_descriptor=-1,delegate=0,host_environment=0,host_system=0;const char* host_version=nullptr;const char* ca_bundle_path=nullptr;const char* app_id=nullptr;const char* app_version=nullptr;};
struct Result {void** confidence_masks=nullptr;std::uint32_t confidence_masks_count=0;void* category_mask=nullptr;std::uint32_t has_category_mask=0;float* quality_scores=nullptr;std::uint32_t quality_scores_count=0;};
using Callback=void(*)(std::int32_t,Result*,void*,std::int64_t);
struct Options {BaseOptions base_options;int running_mode=1;const char* display_names_locale="";bool output_confidence_masks=true,output_category_mask=false;Callback callback=nullptr;};
struct Strings {char** strings=nullptr;int count=0;};
struct Landmark {float x=0,y=0,z=0;bool has_visibility=false;float visibility=0;bool has_presence=false;float presence=0;const char* name=nullptr;};
struct Landmarks {Landmark* landmarks=nullptr;std::uint32_t count=0;};
struct FaceResult {Landmarks* landmarks=nullptr;std::uint32_t count=0;void* blendshapes=nullptr;std::uint32_t blendshape_count=0;void* matrices=nullptr;std::uint32_t matrix_count=0;};
struct FaceOptions {BaseOptions base_options;int running_mode=1,num_faces=1;float detection=.5f,presence=.5f,tracking=.5f;bool blendshapes=false,matrices=false;void(*callback)()=nullptr;};
struct PoseResult {void** masks=nullptr;std::uint32_t count=0;Landmarks* landmarks=nullptr;std::uint32_t landmarks_count=0;Landmarks* world=nullptr;std::uint32_t world_count=0;};
struct PoseOptions {BaseOptions base_options;int running_mode=1,num_poses=4;float detection=.5f,presence=.5f,tracking=.5f;bool masks=true;void(*callback)()=nullptr;};
static_assert(sizeof(Landmark)==40&&sizeof(Landmarks)==16&&sizeof(FaceOptions)==104&&sizeof(PoseOptions)==104&&sizeof(FaceResult)==48&&sizeof(PoseResult)==48,"Verified native people ABI");
static_assert(sizeof(BaseOptions)==72&&sizeof(Options)==104&&sizeof(Result)==48,"Verified arm64 MediaPipe 1.1.1 ABI");
}
