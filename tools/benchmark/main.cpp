#include "fstr/core.hpp"
#include "fstr/ort_backend.hpp"
#include "json.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using json=nlohmann::json;
namespace {
using Clock=std::chrono::steady_clock;
double ms(Clock::time_point t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
}
int main(int argc,char** argv) {
    try {
        if(argc!=9) {std::cerr<<"Usage: fstr_benchmark manifest width height input.rgba32f output.rgba32f runs enhance look.cube-or-none\n";return 2;}
        std::size_t w=std::stoull(argv[2]),h=std::stoull(argv[3]),runs=std::stoull(argv[6]);
        if(!w||!h||w>4096||h>4096||w>2097152/h||!runs||runs>20) throw std::runtime_error("Invalid dimensions or runs");
        std::vector<float> input(w*h*4);
        std::ifstream file(argv[4],std::ios::binary);
        if(!file.read(reinterpret_cast<char*>(input.data()),static_cast<std::streamsize>(input.size()*sizeof(float))) ||
           file.peek()!=std::char_traits<char>::eof()) throw std::runtime_error("Input byte size mismatch");
        auto start=Clock::now();auto backend=fstr::load_ort_cpu(argv[1]);double model_load=ms(start);
        fstr::RenderSettings settings;settings.enhance=std::stod(argv[7]);
        if(std::string(argv[8])!="none") {std::ifstream cube(argv[8]);settings.look=std::make_shared<fstr::LUT>(fstr::LUT::parse(cube));}
        fstr::FrameView frame{input.data(),input.size(),w,h,w*4};
        fstr::RenderContext context;context.backend=backend;context.cache_budget_bytes=0; // Bench inference, not cache hits.
        json timings=json::array();fstr::RenderResult result;
        for(std::size_t i=0;i<runs;++i) {
            auto t=Clock::now();result=fstr::render({frame,0},settings,context);
            auto elapsed=ms(t);
            if(result.status!=fstr::Status::Ok) throw std::runtime_error(result.message);
            timings.push_back({{"total_ms",elapsed},{"preparation_ms",result.preparation_ms},{"inference_ms",result.inference_ms},{"look_mix_ms",result.look_mix_ms},{"cache_hit",result.restoration_cache_hit}});
        }
        std::ofstream output(argv[5],std::ios::binary);
        output.write(reinterpret_cast<const char*>(result.rgba.data()),static_cast<std::streamsize>(result.rgba.size()*sizeof(float)));
        if(!output) throw std::runtime_error("Output write failed");
        std::cout<<json{{"width",w},{"height",h},{"bpc",32},{"backend",backend->identity()},{"model_load_ms",model_load},
          {"model_threads",4},{"active_inferences",1},{"enhance",settings.enhance},{"look",std::string(argv[8])},
          {"runs",timings},{"cache_disabled",true},{"includes_AE",false},{"includes_load_in_frame_time",false},
          {"hardware_threads",std::thread::hardware_concurrency()}}.dump(2)<<"\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<"ERROR: "<<error.what()<<"\n";return 1;}
}
