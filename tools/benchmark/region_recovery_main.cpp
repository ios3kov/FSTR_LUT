// Offline replay of actual detector observations against the same native recovery code.
#include "fstr/regions.hpp"
#include <fstream>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
int main(int argc,char**argv){try{
 if(argc!=4)throw std::invalid_argument("sequence.rgba8 regions.txt output-prefix (540x960,156frames)");
 constexpr int width=540,height=960,count=156;constexpr std::size_t bytes=width*height*4;
 std::ifstream metadata(argv[2]);std::vector<std::vector<fstr::ColorRegion>> regions(count);
 for(auto& frame:regions){int n=-1;if(!(metadata>>n)||n<0||n>16)throw std::runtime_error("Invalid observations");for(int j=0;j<n;++j){fstr::ColorRegion r;if(!(metadata>>r.x0>>r.y0>>r.x1>>r.y1))throw std::runtime_error("Invalid box");frame.push_back(r);}}
 std::ifstream input(argv[1],std::ios::binary);std::ofstream output(std::string(argv[3])+".f32",std::ios::binary);
 for(int i=0;i<count;++i){std::vector<fstr::RegionFrame> window;
  for(int j=std::max(0,i-2);j<=std::min(count-1,i+2);++j){fstr::AnalysisImage im;im.width=width;im.height=height;im.rgba.resize(bytes);input.seekg(std::size_t(j)*bytes);input.read(reinterpret_cast<char*>(im.rgba.data()),bytes);if(!input)throw std::runtime_error("Short input");window.push_back({j-i,std::move(im),regions[j]});}
  auto out=fstr::recover_color_regions(window);std::reverse(window.begin(),window.end());auto reverse=fstr::recover_color_regions(window);if(out.size()!=reverse.size())throw std::runtime_error("Order dependence");
  for(std::size_t j=0;j<out.size();++j)if(out[j].x0!=reverse[j].x0||out[j].y0!=reverse[j].y0||out[j].x1!=reverse[j].x1||out[j].y1!=reverse[j].y1)throw std::runtime_error("Order dependence");
  std::cout<<i<<' '<<regions[i].size()<<' '<<out.size()<<'\n';
  for(int y=0;y<640;++y)for(int x=0;x<360;++x){float v=fstr::color_region_weight((x+.5)/360,(y+.5)/640,3840,out);output.write(reinterpret_cast<char*>(&v),sizeof(v));}
 }
 if(!output)throw std::runtime_error("Output write");return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
