#include "pixel_digest.hpp"
#include "picosha2.h"
#include <vector>
#include <random>
#include <iostream>
#include <cstdint>
#include <cstring>
template<class T>struct Pixel {T alpha,red,green,blue;};
void check(bool v){if(!v)throw std::runtime_error("Pixel digest contract failed");}
template<class T>void run(){using P=Pixel<T>;std::mt19937 rng(5821);for(std::size_t width:{0U,1U,17U,960U,1920U})for(std::size_t padding:{0U,7U,64U}){
 // All object bytes are hashed, including signed zero/NaN float payloads.
 const auto stride=width*sizeof(P)+padding;std::vector<unsigned char> data(stride*3+1);for(auto&v:data)v=std::uint8_t(rng());std::string prefix="source-metadata /8/16/32 ";fstr::ByteSha256 fast;fast.update(prefix.data(),prefix.size());picosha2::hash256_one_by_one old;old.init();old.process(prefix.begin(),prefix.end());
 for(std::size_t y=0;y<3;++y){auto row=data.data()+y*stride;fstr::ae::hash_argb_row<P>(fast,row,width);for(std::size_t x=0;x<width;++x)for(std::size_t channel=0;channel<4;++channel){auto start=row+x*sizeof(P)+channel*sizeof(T);old.process(start,start+sizeof(T));}}old.finish();check(fast.finish()==picosha2::get_hash_hex_string(old));
 }}
int main(){try{run<std::uint8_t>();run<std::uint16_t>();run<float>();fstr::ByteSha256 hash;bool failed=false;try{fstr::ae::hash_argb_row<Pixel<float>>(hash,nullptr,std::size_t(-1));}catch(const std::invalid_argument&){failed=true;}check(failed);std::cout<<"PASS45packed ARGB row/padding/source-prefix cases and overflow\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
