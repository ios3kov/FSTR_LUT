#include "fstr/native_scene_identity.hpp"
#include "fstr/verified_sha256.hpp"
#include "onnxruntime_cxx_api.h"
#include "json.hpp"
#include "picosha2.h"
#include <filesystem>
#include <fstream>
#include <dlfcn.h>
#ifdef FSTR_SCENE_COREML
#include <sys/sysctl.h>
#endif
namespace fstr { namespace {
using Path=std::filesystem::path;
std::string bytes(const Path& p,std::uintmax_t budget){
    if(!p.is_absolute()||!std::filesystem::is_regular_file(std::filesystem::symlink_status(p))||std::filesystem::file_size(p)>budget)
        throw std::invalid_argument("Native identity manifest budget");
    std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Native identity manifest unavailable");
    std::string out(std::istreambuf_iterator<char>(f),{});if(f.bad())throw std::runtime_error("Native identity manifest read failed");return out;
}
std::string digest(const Path& p,const std::function<void()>& cancel){
    if(cancel)cancel();std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Native identity asset unavailable");
    const auto result=verified_sha256(f);if(cancel)cancel();return result;
}
void verify(const Path& p,const std::string& expected,std::uintmax_t budget,const std::function<void()>& cancel){
    if(expected.size()!=64||!p.is_absolute()||!std::filesystem::is_regular_file(std::filesystem::symlink_status(p))||std::filesystem::file_size(p)>budget)
        throw std::invalid_argument("Native identity asset contract");
    if(digest(p,cancel)!=expected)throw std::runtime_error("Native identity asset checksum mismatch");
}
}
std::string verified_native_scene_policy(const std::string& directory,const std::function<void()>& cancel){
    const Path root(directory);if(!root.is_absolute())throw std::invalid_argument("Absolute native resources required");
    const auto runtime_bytes=bytes(root/"runtime.json",4096),scene_bytes=bytes(root/"scene/manifest.json",131072),face_bytes=bytes(root/"detector/manifest.json",16384);
    const auto runtime=nlohmann::json::parse(runtime_bytes),scene=nlohmann::json::parse(scene_bytes),face=nlohmann::json::parse(face_bytes);
    if(runtime.at("schema")!=1||scene.at("schema")!=1||face.at("schema")!=1)throw std::invalid_argument("Native identity schema mismatch");
    constexpr std::uintmax_t big=512ULL*1024*1024;
    verify(root/"libmediapipe.dylib",runtime.at("sha256"),big,cancel);
    verify(root/"skin.tflite","c6748b1253a99067ef71f7e26ca71096cd449baefa8f101900ea23016507e0e0",big,cancel);
    verify(root/"face.task","64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff",big,cancel);
    verify(root/"pose.task","4eaa5eb7a98365221087693fcc286334cf0858e2eb6e15b506aa4a7ecdcec4ad",big,cancel);
    if(scene.at("input").at("shape")!=std::vector<int>{1,3,512,512}||scene.at("output").at("shape")!=std::vector<int>{1,150,512,512}||scene.at("source").at("files").at("model.safetensors")!="cd55b1d36c6602d34f8cd300d7f788ea0fa5c3ed928e9ac98f353ecb31c7fa1d"||scene.at("input").at("mean")!=std::vector<double>{.485,.456,.406}||scene.at("input").at("std")!=std::vector<double>{.229,.224,.225})throw std::invalid_argument("Native identity scene contract");
    const auto& labels=scene.at("output").at("id2label");if(labels.size()!=150||labels.at("12")!="person")throw std::invalid_argument("Native identity labels");
    for(int i=0;i<150;++i)(void)labels.at(std::to_string(i)).get<std::string>();
    verify(root/"scene/scene.onnx",scene.at("onnx_sha256"),big,cancel);
    if(face.at("source_sha256")!="ebafce4e3c118d6554634be5c27ab333b4c047a9a8c3faf1d7cf93101c22f0f0")throw std::invalid_argument("Native identity detector provenance");
    verify(root/"detector/yunet.onnx",face.at("sha256"),4*1024*1024,cancel);
    const auto* version=OrtGetApiBase()->GetVersionString();if(!version||std::string(version)!="1.23.2")throw std::runtime_error("Unsupported native runtime");
    Dl_info info{};if(!dladdr(reinterpret_cast<void*>(&OrtGetApiBase),&info)||!info.dli_fname)throw std::runtime_error("Native runtime identity unavailable");
    auto runtime_identity=digest(info.dli_fname,cancel);
#ifdef FSTR_SCENE_COREML
    char os[128]{};std::size_t size=sizeof(os);if(sysctlbyname("kern.osversion",os,&size,nullptr,0)||!size||size>sizeof(os)||os[size-1]!=0)throw std::runtime_error("CoreML OS identity unavailable");
    runtime_identity+="/CoreML-MLProgram-ALL-static-v1/";runtime_identity+=os;
#endif
    // The fingerprint covers the unchanged observation algorithms and their
    // full constructors. This verification-only reader cannot produce masks.
    return picosha2::hash256_hex_string(std::string("FSTR_NATIVE_AUTO_V1\n" FSTR_NATIVE_ANALYSIS_ID)+runtime_identity+runtime_bytes+scene_bytes+face_bytes);
}
}
