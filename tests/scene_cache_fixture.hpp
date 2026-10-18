#pragma once
#include "fstr/scene_cache.hpp"
#include "json.hpp"
#include "picosha2.h"
#include <filesystem>
#include <fstream>
#include <cstring>
namespace scene_fixture {
inline std::string write(const std::filesystem::path&root,const fstr::AnalysisImage&image,double height,const std::string&policy,bool stable=false){
 const auto key=fstr::scene_cache_key(image,height,policy);std::filesystem::create_directories(root);
 auto payload=[&](const std::string&name,const std::vector<float>&values){std::string bytes(values.size()*4,'\0');for(std::size_t i=0;i<values.size();++i){std::uint32_t bits;std::memcpy(&bits,&values[i],4);for(std::size_t c=0;c<4;++c)bytes[i*4+c]=char((bits>>(c*8))&255);}std::ofstream file(root/name,std::ios::binary);file.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));file.close();if(!file)throw std::runtime_error("Fixture write");return picosha2::hash256_hex_string(bytes);};
 const std::size_t pixels=std::size_t(image.width)*std::size_t(image.height);std::vector<float> weights(pixels*8),masks(pixels,1),samples;
 for(std::size_t p=0;p<pixels;++p)weights[p*8]=1;
 for(int n=0;n<40;++n)for(float v:{.23f,.12f,.07f,1.f})samples.push_back(v);
 const auto wh=payload(key+".weights",weights),mh=payload(key+"-p0.mask",masks),sh=payload(key+"-p0.samples",samples);
 nlohmann::json j={{"version",1},{"key",key},{"policy",policy},{"width",image.width},{"height",image.height},{"source_height",height},{"scene_sha256",wh},{"people",nlohmann::json::array({{{"samples",40},{"mask_sha256",mh},{"samples_sha256",sh}}})}};
 if(stable){j["version"]=2;j["people"][0]["toning_strength"]=.5;j["people"][0]["toning_support_sha256"]=payload(key+"-p0.support",std::vector<float>(pixels,.75f));}
 std::ofstream file(root/(key+".fstrscene"));file<<j.dump();file.close();if(!file)throw std::runtime_error("Fixture header");return key;
}
}
