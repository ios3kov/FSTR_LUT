#include "fstr/sdr_prepare.hpp"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace fstr;
namespace {
std::size_t integer(const char* s){std::size_t n=0;const auto v=std::stoull(s,&n);if(n!=std::string(s).size()||!v||v>16384)throw std::invalid_argument("Invalid extent");return std::size_t(v);}
std::vector<float> read(const char* name,std::size_t bytes){if(std::filesystem::file_size(name)!=bytes)throw std::invalid_argument("Raw frame size differs");std::vector<float> v(bytes/4);std::ifstream f(name,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),std::streamsize(bytes));if(!f)throw std::runtime_error("Raw frame read failed");return v;}
}
int main(int argc,char** argv){try{
    if(argc<6)throw std::invalid_argument("Usage: analyze width height srgb|linear|gamma24 anchor1.rgba32f [anchor2..16] OR render width height transfer recipe.json input.rgba32f output.rgba32f; known converted SDR only");
    const std::string mode=argv[1],transfer=argv[4];const auto w=integer(argv[2]),h=integer(argv[3]);ColorContext ctx;if(w>ctx.max_pixels/h)throw std::invalid_argument("Pixel budget exceeded");
    Transfer t;if(transfer=="srgb")t=Transfer::SRGB;else if(transfer=="linear")t=Transfer::Linear;else if(transfer=="gamma24")t=Transfer::Gamma24;else throw std::invalid_argument("Unknown transfer");
    auto frame=[&](const std::vector<float>& v){return FrameView{v.data(),v.size(),w,h,w*4,t};};
    const auto bytes=w*h*4*sizeof(float);
    if(mode=="analyze"){
        if(argc>21)throw std::invalid_argument("Anchor budget exceeded");
        std::vector<std::vector<float>> storage;for(int i=5;i<argc;++i)storage.push_back(read(argv[i],bytes));std::vector<FrameView> frames;for(const auto& v:storage)frames.push_back(frame(v));
        const auto a=analyze_sdr_shot(frames,ctx);const auto s=a.settings;const char* names[]{"prepared","balanced","ambiguous","inconsistent"};
        nlohmann::json j{{"schema",1},{"algorithm","sdr-shot-v2-prototype"},{"decision",names[int(a.decision)]},{"contrast",s.contrast},{"brightness_bias",s.brightness_bias},{"saturation",s.saturation},{"amount",s.amount},{"low",a.low},{"median",a.median},{"high",a.high},{"flatness",a.flatness},{"samples",a.samples}};
        std::cout<<j.dump(2)<<'\n';
    }else if(mode=="render"){
        if(argc!=8)throw std::invalid_argument("Render arguments differ");
        if(std::filesystem::file_size(argv[5])>16384)throw std::invalid_argument("Recipe budget exceeded");
        std::ifstream f(argv[5]);nlohmann::json j;f>>j;if(j.at("schema")!=1||j.at("algorithm")!="sdr-shot-v2-prototype")throw std::invalid_argument("Unknown preparation recipe");
        SdrPreparation s{j.at("contrast"),j.at("brightness_bias"),j.at("saturation"),j.at("amount")};validate_sdr_preparation(s);auto v=read(argv[6],bytes);auto out=render_sdr_preparation(frame(v),s,ctx);if(out.status!=Status::Ok)throw std::runtime_error("SDR preparation render failed");
        std::ofstream dst(argv[7],std::ios::binary);dst.write(reinterpret_cast<const char*>(out.rgba.data()),std::streamsize(bytes));dst.close();if(!dst)throw std::runtime_error("Preparation output write failed");
    }else throw std::invalid_argument("Unknown command");
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
