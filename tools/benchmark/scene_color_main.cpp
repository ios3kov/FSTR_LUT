// Research CLI. Input straight linear Rec709 RGBA32F + interleaved8-role32F.
#include "fstr/scene_color.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{
 if(argc!=7)throw std::invalid_argument("look.cube width height input.rgba32f weights.f32 output.rgba32f");
 const int w=std::stoi(argv[2]),h=std::stoi(argv[3]);if(w<1||h<1||w>8192||h>8192||std::size_t(w)*h>8388608)throw std::invalid_argument("Frame dimensions");
 std::ifstream cube(argv[1]);auto lut=std::make_shared<fstr::LUT>(fstr::LUT::parse(cube));fstr::SceneLook grade(lut,1);
 if(std::filesystem::exists(argv[6]))throw std::invalid_argument("Output must be a new file; existing inputs/outputs are preserved");
 std::ifstream input(argv[4],std::ios::binary),masks(argv[5],std::ios::binary);std::ofstream output(argv[6],std::ios::binary);
 for(std::size_t i=0;i<std::size_t(w)*h;++i){std::array<float,4> p;fstr::SceneWeights m;input.read(reinterpret_cast<char*>(p.data()),sizeof(p));masks.read(reinterpret_cast<char*>(m.data()),sizeof(m));if(!input||!masks)throw std::runtime_error("Truncated frame");auto v=grade.apply(p,m);output.write(reinterpret_cast<char*>(v.data()),sizeof(v));}
 output.flush();output.close();
 if(input.peek()!=EOF||masks.peek()!=EOF||!output)throw std::runtime_error("Exact frame sizes/output required");return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
