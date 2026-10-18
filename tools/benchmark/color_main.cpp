#include "fstr/color.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace fstr;

int main(int argc,char** argv) {
    try {
        if(argc!=15)throw std::invalid_argument("Usage: fstr_color_render width height input.rgba32f output.rgba32f srgb|linear|gamma24 source-key exposure contrast saturation shoulder look.cube|none look_amount mix straight|premult; source-key: host, sony-slog3-cine, arri-logc3-ei800, arri-logc4, panasonic-vlog, acescct, acescg, linear-rec709");
        auto integer=[](const char* text){std::size_t count=0;auto v=std::stoull(text,&count);if(count!=std::string(text).size())throw std::invalid_argument("Invalid integer");return v;};
        auto number=[](const char* text){std::size_t count=0;double v=std::stod(text,&count);if(count!=std::string(text).size())throw std::invalid_argument("Invalid number");return v;};
        const auto width=integer(argv[1]),height=integer(argv[2]);ColorContext context;
        if(!width||!height||width>16384||height>16384||width>context.max_pixels/height)throw std::invalid_argument("CLI frame budget exceeded");
        const auto samples=static_cast<std::size_t>(width*height*4),bytes=samples*sizeof(float);
        if(std::filesystem::file_size(argv[3])!=bytes)throw std::invalid_argument("Wrong raw input byte count");
        std::vector<float> input(samples);std::ifstream stream(argv[3],std::ios::binary);
        stream.read(reinterpret_cast<char*>(input.data()),static_cast<std::streamsize>(bytes));if(!stream)throw std::runtime_error("Incomplete raw input read");
        FrameView frame{input.data(),samples,static_cast<std::size_t>(width),static_cast<std::size_t>(height),static_cast<std::size_t>(width)*4};
        const std::string transfer=argv[5],source=argv[6];
        if(transfer=="srgb")frame.transfer=Transfer::SRGB;
        else if(transfer=="linear")frame.transfer=Transfer::Linear;
        else if(transfer=="gamma24")frame.transfer=Transfer::Gamma24;
        else throw std::invalid_argument("Unknown host transfer");
        ColorSettings settings;
        bool supported=false;for(int i=0;i<source_profile_count;++i)if(source==source_profile_keys[i]){settings.source=static_cast<SourceProfile>(i);supported=true;break;}
        if(!supported)throw std::invalid_argument("Unknown source profile");
        settings.grade={number(argv[7]),number(argv[8]),number(argv[9])};settings.shoulder=number(argv[10]);
        if(std::string(argv[11])!="none") {std::ifstream cube(argv[11]);if(!cube)throw std::runtime_error("Cannot load look");settings.look=std::make_shared<LUT>(LUT::parse(cube));}
        settings.look_amount=number(argv[12]);settings.mix=number(argv[13]);
        if(std::string(argv[14])=="premult")frame.alpha=Alpha::Premultiplied;
        else if(std::string(argv[14])!="straight")throw std::invalid_argument("Alpha must be straight or premult");
        auto result=render_color({frame,0},settings,context);
        if(result.status!=Status::Ok)throw std::runtime_error(result.message.empty()?"Color processing failed":result.message);
        std::ofstream output(argv[4],std::ios::binary);output.write(reinterpret_cast<const char*>(result.rgba.data()),static_cast<std::streamsize>(bytes));
        output.flush();if(!output)throw std::runtime_error("Incomplete raw output write");
        std::cout<<"PASS color-only render; "<<width<<"x"<<height<<"; no inference backend\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}
}
