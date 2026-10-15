// Local native detector probe: packed RGBA8, explicit source height, no AE/Python runtime.
#include "fstr/regions.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{
 if(argc!=6)throw std::invalid_argument("width height source-height input.rgba8 output.mask-f32");
 fstr::AnalysisImage image;image.width=std::stoi(argv[1]);image.height=std::stoi(argv[2]);
 if(image.width<1||image.height<1||image.width>2048||image.height>2048)throw std::invalid_argument("dimensions");
 double source=std::stod(argv[3]);image.rgba.resize(std::size_t(image.width)*image.height*4);
 std::ifstream input(argv[4],std::ios::binary);input.read(reinterpret_cast<char*>(image.rgba.data()),image.rgba.size());
 if(!input||input.peek()!=EOF)throw std::runtime_error("exact input size required");
 auto regions=fstr::detect_color_regions(image,source);
 std::ofstream output(argv[5],std::ios::binary);std::cout<<"{\"regions\":[";
 bool first=true;for(const auto&r:regions){if(!first)std::cout<<',';first=false;std::cout<<"{\"box\":["<<r.x0<<','<<r.y0<<','<<r.x1<<','<<r.y1<<"],\"detail\":"<<r.detail<<",\"holes\":"<<r.holes.size()<<'}';}
 std::cout<<"]}\n";
 for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x){float v=fstr::color_region_weight((x+.5)/image.width,(y+.5)/image.height,source,regions);output.write(reinterpret_cast<char*>(&v),sizeof(v));}
 if(!output)throw std::runtime_error("mask write failed");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
