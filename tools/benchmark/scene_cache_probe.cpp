#include "fstr/scene_cache.hpp"
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=6)throw std::invalid_argument("cache-directory policy-sha width height source-height < prepared.rgba8");
 const int width=std::stoi(argv[3]),height=std::stoi(argv[4]);if(width<1||height<1||width>2048||height>2048||std::size_t(width)*std::size_t(height)>1048576)throw std::invalid_argument("Image budget");
 fstr::AnalysisImage image{width,height,{}};image.rgba.resize(std::size_t(width)*std::size_t(height)*4);std::cin.read(reinterpret_cast<char*>(image.rgba.data()),static_cast<std::streamsize>(image.rgba.size()));if(!std::cin||std::cin.peek()!=EOF)throw std::runtime_error("Exact RGBA8 input required");
 fstr::SceneCache cache(argv[1],argv[2]);auto result=cache.get(image,std::stod(argv[5]));if(!result)throw std::runtime_error("Scene cache miss");
 std::cout<<"PASS "<<result->scene.size()<<" pixels, "<<result->people.size()<<" people, GUID "<<cache.guid()<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
