// Research CLI: straight linear Rec709 RGBA32F, float32 mask and RGB+confidence samples.
// Samples are explicit shot-level analysis output, not recomputed from each frame.
#include "fstr/skin_tone.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <vector>
int main(int argc,char**argv){try{
 if(argc!=9)throw std::invalid_argument("look.cube width height frame.rgba32f mask.f32 shot-samples.rgbw32f output.rgba32f strength");
 const int w=std::stoi(argv[2]),h=std::stoi(argv[3]);if(w<1||h<1||w>8192||h>8192||std::size_t(w)*h>8388608)throw std::invalid_argument("Frame dimensions");
 const std::size_t count=std::size_t(w)*h;
 const auto bytes=std::filesystem::file_size(argv[6]);if(bytes%16||bytes/16>1048576)throw std::invalid_argument("Sample file size");
 if(std::filesystem::file_size(argv[4])!=count*16||std::filesystem::file_size(argv[5])!=count*4)throw std::invalid_argument("Exact frame/mask sizes required");
 if(std::filesystem::exists(argv[7]))throw std::invalid_argument("Output exists; preserve inputs");
 std::ifstream cube(argv[1]);auto lut=std::make_shared<fstr::LUT>(fstr::LUT::parse(cube));fstr::SkinToneLook grade(lut,std::stod(argv[8]));
 std::ifstream input(argv[4],std::ios::binary),masks(argv[5],std::ios::binary),samples_file(argv[6],std::ios::binary);
 std::vector<fstr::SkinSample> samples;for(std::uintmax_t i=0;i<bytes/16;++i){std::array<float,4> v{};samples_file.read(reinterpret_cast<char*>(v.data()),sizeof(v));if(!samples_file)throw std::runtime_error("Sample read failed");samples.push_back({{v[0],v[1],v[2]},v[3]});}
 const auto reference=grade.reference(samples);
 // Compute before creating output so malformed analysis does not leave partial frames.
 std::vector<std::array<float,4>> result;result.reserve(count);
 for(std::size_t i=0;i<count;++i){std::array<float,4> p{};float mask=0;input.read(reinterpret_cast<char*>(p.data()),sizeof(p));masks.read(reinterpret_cast<char*>(&mask),sizeof(mask));if(!input||!masks)throw std::runtime_error("Frame read failed");result.push_back(grade.apply(p,reference,mask));}
 std::ofstream output(argv[7],std::ios::binary);for(const auto&p:result)output.write(reinterpret_cast<const char*>(p.data()),sizeof(p));output.close();if(!output)throw std::runtime_error("Output write failed");
 std::cout<<"Reference samples "<<reference.samples<<", confidence "<<reference.confidence<<"\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
