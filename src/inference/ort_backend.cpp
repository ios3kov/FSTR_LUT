#include "fstr/ort_backend.hpp"
#include "onnxruntime_cxx_api.h"
#include "json.hpp"
#include "picosha2.h"
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <thread>
namespace fstr {
namespace {
using json=nlohmann::json;
std::vector<unsigned char> read_bounded(const std::filesystem::path& path,std::size_t maximum) {
    std::ifstream file(path,std::ios::binary|std::ios::ate);
    if(!file) throw std::runtime_error("Cannot read model artifact");
    auto length=file.tellg();
    if(length<0 || static_cast<std::uint64_t>(length)>maximum) throw std::runtime_error("Artifact exceeds size budget");
    std::vector<unsigned char> bytes(static_cast<std::size_t>(length)); file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),length)) throw std::runtime_error("Truncated model artifact");
    return bytes;
}
class OrtCPU final : public InferenceBackend {
    Ort::Env env_{ORT_LOGGING_LEVEL_WARNING,"FSTR-LUT"};
    Ort::Session session_{nullptr};
    std::timed_mutex admission_;
    std::string digest_,input_name_,output_name_;
public:
    explicit OrtCPU(const std::filesystem::path& path) {
        if(std::string(Ort::GetVersionString())!="1.23.2") throw std::runtime_error("Runtime version must be 1.23.2");
        auto manifest=read_bounded(path,65536);
        auto j=json::parse(manifest.begin(),manifest.end());
        if(j.at("schema")!=1 || j.at("architecture")!="ffdnet-color-trunk" ||
           j.at("input_space")!="sRGB" || j.at("normalization")!="identity-0-1" ||
           j.at("layout")!="NCHW" || j.at("input_channels")!=13 || j.at("output_channels")!=12 ||
           j.at("output_scale")!=1 || j.at("temporal_radius")!=0 || j.at("precision")!="FP32" ||
           j.at("padding")!="replicate-bottom-right-to-even" || j.at("shuffle_order")!="c*4+dy*2+dx" ||
           j.at("max_pixels")!=2097152)
            throw std::runtime_error("Unsupported model manifest contract");
        auto filename=j.at("file").get<std::string>();
        auto basename=std::filesystem::path(filename);
        if(filename.empty() || basename.is_absolute() || basename.filename()!=basename || basename.extension()!=".onnx")
            throw std::runtime_error("Model path must be a local ONNX filename");
        digest_=j.at("sha256").get<std::string>();
        auto graph=read_bounded(path.parent_path()/basename,16u*1024u*1024u);
        if(digest_.size()!=64 || picosha2::hash256_hex_string(graph)!=digest_)
            throw std::runtime_error("Model checksum mismatch");
        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(4);options.SetInterOpNumThreads(1);
        options.SetExecutionMode(ORT_SEQUENTIAL);
        options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        // In-memory loading disallows reliance on paths to arbitrary external tensors.
        session_=Ort::Session(env_,graph.data(),graph.size(),options);
        if(session_.GetInputCount()!=1 || session_.GetOutputCount()!=1)
            throw std::runtime_error("Expected one input and one output");
        auto input_type=session_.GetInputTypeInfo(0);
        auto output_type=session_.GetOutputTypeInfo(0);
        auto in_info=input_type.GetTensorTypeAndShapeInfo();
        auto out_info=output_type.GetTensorTypeAndShapeInfo();
        auto in_shape=in_info.GetShape(),out_shape=out_info.GetShape();
        if(in_info.GetElementType()!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
           out_info.GetElementType()!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT ||
           in_shape.size()!=4 || out_shape.size()!=4 || in_shape[0]!=1 || out_shape[0]!=1 ||
           in_shape[1]!=13 || out_shape[1]!=12)
            throw std::runtime_error("Unexpected graph tensor contract");
        Ort::AllocatorWithDefaultOptions allocator;
        input_name_=session_.GetInputNameAllocated(0,allocator).get();
        output_name_=session_.GetOutputNameAllocated(0,allocator).get();
    }
    std::string identity() const override {return digest_+":ort1.23.2:cpu:fp32";}
    void infer(const PackedTensor& input,std::vector<float>& output,const std::atomic_bool* cancelled) override {
        auto stopped=[&](){return cancelled && cancelled->load();};
        std::unique_lock<std::timed_mutex> lock(admission_,std::defer_lock);
        while(!lock.try_lock_for(std::chrono::milliseconds(10)))
            if(stopped()) throw std::runtime_error("Cancelled while waiting for inference");
        if(stopped()) throw std::runtime_error("Cancelled");
        if(!input.half_width || !input.half_height || input.half_width>2048 || input.half_height>2048 ||
           input.half_width>524288/input.half_height ||
           input.data.size()!=13*input.half_width*input.half_height)
            throw std::invalid_argument("Invalid packed model tensor");
        std::array<std::int64_t,4> in_shape{1,13,static_cast<std::int64_t>(input.half_height),static_cast<std::int64_t>(input.half_width)};
        auto out_shape=in_shape;out_shape[1]=12;
        output.resize(12*input.half_width*input.half_height);
        auto memory=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);
        auto in=Ort::Value::CreateTensor<float>(memory,const_cast<float*>(input.data.data()),input.data.size(),in_shape.data(),4);
        auto out=Ort::Value::CreateTensor<float>(memory,output.data(),output.size(),out_shape.data(),4);
        const char* inputs[]={input_name_.c_str()},*outputs[]={output_name_.c_str()};
        Ort::RunOptions options;std::mutex monitor_mutex;std::condition_variable cv;bool done=false;
        std::exception_ptr monitor_error;
        std::thread monitor;
        if(cancelled) monitor=std::thread([&](){
            std::unique_lock<std::mutex> guard(monitor_mutex);
            while(!done) {
                if(stopped()) {
                    try {options.SetTerminate();} catch(...) {monitor_error=std::current_exception();}
                    return;
                }
                cv.wait_for(guard,std::chrono::milliseconds(5));
            }
        });
        auto finish=[&](){
            {std::lock_guard<std::mutex> guard(monitor_mutex);done=true;}
            cv.notify_one();if(monitor.joinable())monitor.join();
        };
        try {session_.Run(options,inputs,&in,1,outputs,&out,1);}
        catch(...) {finish();throw;}
        finish();
        if(monitor_error) std::rethrow_exception(monitor_error);
        if(stopped()) throw std::runtime_error("Cancelled during inference");
    }
};
}
std::shared_ptr<InferenceBackend> load_ort_cpu(const std::filesystem::path& path) {
    return std::make_shared<OrtCPU>(path);
}
}
