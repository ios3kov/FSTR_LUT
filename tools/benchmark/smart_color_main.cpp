#include "fstr/smart_color.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <algorithm>

// Bounded offline stream, native little-endian files for same-Mac research only.
// rgba32f: opaque straight LINEAR Rec709. mask32f: one weight per pixel.
// balance64f: exposure,rGain,gGain,bGain for each frame. No source guessing.
int main(int argc,char** argv) {
    try {
        if(argc!=10)throw std::invalid_argument("Usage: fstr_smart_render width height input.rgba32f mask.f32|none balance.f64 output.rgba32f look.cube|none strength protection");
        const auto integer=[](const char* t){std::size_t used=0;auto v=std::stoull(t,&used);if(used!=std::string(t).size())throw std::invalid_argument("Invalid integer");return v;};
        const auto number=[](const char* t){std::size_t used=0;auto v=std::stod(t,&used);if(used!=std::string(t).size())throw std::invalid_argument("Invalid number");return v;};
        const auto w=integer(argv[1]),h=integer(argv[2]);
        if(!w||!h||w>16384||h>16384||w>8388608/h)throw std::invalid_argument("Smart frame budget exceeded");
        const std::size_t n=static_cast<std::size_t>(w*h),bytes=n*4*sizeof(float);
        const auto input_bytes=std::filesystem::file_size(argv[3]);
        if(input_bytes%bytes||!input_bytes||input_bytes/bytes>512)throw std::invalid_argument("Invalid stream size/frame budget");
        const auto frames=input_bytes/bytes;const bool masked=std::string(argv[4])!="none";
        if(std::filesystem::file_size(argv[5])!=frames*4*sizeof(double)||(masked&&std::filesystem::file_size(argv[4])!=frames*n*sizeof(float)))throw std::invalid_argument("Analysis stream dimensions mismatch");
        // Refuse aliases before opening any output for writing.
        for(int j:{3,4,5,7})if(std::string(argv[j])!="none"&&std::filesystem::weakly_canonical(argv[j])==std::filesystem::weakly_canonical(argv[6]))throw std::invalid_argument("Output aliases an input");
        if(std::filesystem::exists(argv[6]))throw std::invalid_argument("Output exists; choose a fresh path");
        std::shared_ptr<const fstr::LUT> lut;
        if(std::string(argv[7])!="none"){std::ifstream stream(argv[7]);if(!stream)throw std::runtime_error("Cannot load look");lut=std::make_shared<fstr::LUT>(fstr::LUT::parse(stream));}
        const fstr::SmartLook look(lut,number(argv[8]),number(argv[9]));
        std::ifstream input(argv[3],std::ios::binary),params(argv[5],std::ios::binary),mask;
        if(masked)mask.open(argv[4],std::ios::binary);
        if(!input||!params||(masked&&!mask))throw std::runtime_error("Cannot open input");
        std::ofstream output(argv[6],std::ios::binary);
        if(!output)throw std::runtime_error("Cannot open output");
        std::vector<float> rgb(n*4),weights(n,0);
        const auto started=std::chrono::steady_clock::now();
        for(std::uintmax_t f=0;f<frames;++f) {
            double b[4];input.read(reinterpret_cast<char*>(rgb.data()),static_cast<std::streamsize>(bytes));params.read(reinterpret_cast<char*>(b),sizeof b);
            if(masked)mask.read(reinterpret_cast<char*>(weights.data()),static_cast<std::streamsize>(n*sizeof(float)));
            if(!input||!params||(masked&&!mask))throw std::runtime_error("Truncated stream");
            const fstr::SmartBalance balance{b[0],{b[1],b[2],b[3]}};fstr::validate_smart_balance(balance);
            for(std::size_t j=0;j<n;++j) {
                auto* px=rgb.data()+4*j;
                if(px[3]!=1)throw std::invalid_argument("Prototype stream requires opaque alpha1");
                auto v=look.apply(fstr::prepare_smart_color({px[0],px[1],px[2]},balance),weights[j]);
                px[0]=static_cast<float>(v.r);px[1]=static_cast<float>(v.g);px[2]=static_cast<float>(v.b);
            }
            output.write(reinterpret_cast<const char*>(rgb.data()),static_cast<std::streamsize>(bytes));if(!output)throw std::runtime_error("Output write failed");
        }
        output.flush();if(!output)throw std::runtime_error("Output flush failed");
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
        std::cout<<"{\"frames\":"<<frames<<",\"elapsed_ms\":"<<ms<<",\"ms_per_frame\":"<<ms/static_cast<double>(frames)<<"}\n";
        return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
