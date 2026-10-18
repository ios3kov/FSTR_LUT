#include "fstr/scene_cache.hpp"
#include "json.hpp"
#include "picosha2.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <list>
#include <map>
#include <limits>
#include <locale>
#include <mutex>
#include <sstream>
namespace fstr {namespace {
constexpr std::size_t max_entry=64*1024*1024,max_resident=128*1024*1024;
using Json=nlohmann::json;
bool hash_text(const std::string&s){return s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
std::string read(const std::filesystem::path&p,std::size_t expected){
 if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(p))||std::filesystem::file_size(p)!=expected)throw std::runtime_error("Scene cache payload size/type mismatch");
 std::string bytes(expected,'\0');std::ifstream file(p,std::ios::binary);file.read(bytes.data(),static_cast<std::streamsize>(bytes.size()));if(!file||file.peek()!=EOF)throw std::runtime_error("Scene cache read failed");return bytes;
}
std::vector<float> payload(const std::filesystem::path&p,std::size_t count,const Json&sha){
 if(count>max_entry/4)throw std::invalid_argument("Scene payload budget");
 const auto bytes=read(p,count*4);const auto expected=sha.get<std::string>();if(!hash_text(expected)||picosha2::hash256_hex_string(bytes)!=expected)throw std::runtime_error("Scene cache checksum mismatch");
 static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559,"IEEE32 required");
 std::vector<float> result(count);for(std::size_t i=0;i<count;++i){std::uint32_t bits=0;for(std::size_t c=0;c<4;++c)bits|=std::uint32_t(static_cast<unsigned char>(bytes[i*4+c]))<<(c*8);std::memcpy(&result[i],&bits,4);if(!std::isfinite(result[i]))throw std::invalid_argument("Nonfinite scene cache");}return result;
}
void unit(float v){if(v<0||v>1)throw std::invalid_argument("Invalid scene cache probability");}
}
std::string scene_cache_key(const AnalysisImage&i,double height,const std::string&policy){
 if(i.width<1||i.height<1||i.width>2048||i.height>2048||std::size_t(i.width)*std::size_t(i.height)>1048576||i.rgba.size()!=std::size_t(i.width)*std::size_t(i.height)*4||!std::isfinite(height)||height<=0||!hash_text(policy))throw std::invalid_argument("Invalid scene cache identity");
 std::ostringstream meta;meta.imbue(std::locale::classic());meta<<"FSTR_SCENE_V1\n"<<policy<<'\n'<<i.width<<' '<<i.height<<' '<<std::setprecision(17)<<height<<'\n';auto bytes=meta.str();bytes.append(reinterpret_cast<const char*>(i.rgba.data()),i.rgba.size());return picosha2::hash256_hex_string(bytes);
}
struct SceneCache::Impl {
 bool trace=std::getenv("FSTR_LUT_SCENE_TRACE")&&std::string(std::getenv("FSTR_LUT_SCENE_TRACE"))=="1";std::filesystem::path directory;std::string policy,guid;std::map<std::string,std::string> headers;std::mutex mutex;
 struct Entry {std::string key;std::shared_ptr<const SceneAnalysis> value;std::size_t bytes;};
 std::list<Entry> entries;std::size_t bytes=0;
 Impl(std::string d,std::string p):directory(std::move(d)),policy(std::move(p)){if(!directory.is_absolute()||!hash_text(policy)||!std::filesystem::is_directory(directory))throw std::invalid_argument("Scene cache needs existing absolute directory and policy SHA256");
  for(const auto&e:std::filesystem::directory_iterator(directory)){const auto key=e.path().stem().string();if(e.path().extension()!=".fstrscene"||!hash_text(key)||!std::filesystem::is_regular_file(e.symlink_status()))continue;const auto n=e.file_size();if(n>65536||headers.size()>=256)throw std::invalid_argument("Scene snapshot header budget");headers.emplace(key,picosha2::hash256_hex_string(read(e.path(),static_cast<std::size_t>(n))));}
  std::string identity="FSTR_SCENE_SNAPSHOT_V1\n"+policy+'\n';for(const auto&h:headers)identity+=h.first+":"+h.second+'\n';guid=picosha2::hash256_hex_string(identity);
 }
};
SceneCache::SceneCache(std::string d,std::string p):impl_(new Impl(std::move(d),std::move(p))){}
SceneCache::~SceneCache()=default;
const std::string& SceneCache::guid()const noexcept{return impl_->guid;}
void SceneCache::clear_memory(){std::lock_guard<std::mutex> lock(impl_->mutex);impl_->entries.clear();impl_->bytes=0;}
std::shared_ptr<const SceneAnalysis> SceneCache::get(const AnalysisImage&i,double height){
 auto key=scene_cache_key(i,height,impl_->policy);std::lock_guard<std::mutex> lock(impl_->mutex);
 auto found=std::find_if(impl_->entries.begin(),impl_->entries.end(),[&](const auto&e){return e.key==key;});
 if(found!=impl_->entries.end()){if(impl_->trace)std::fprintf(stderr,"FSTR_SCENE RAM %s\n",key.c_str());auto value=found->value;impl_->entries.splice(impl_->entries.begin(),impl_->entries,found);return value;}
 const auto expected=impl_->headers.find(key);if(expected==impl_->headers.end()){if(impl_->trace)std::fprintf(stderr,"FSTR_SCENE MISS %s\n",key.c_str());return {};}
 const auto path=impl_->directory/(key+".fstrscene");
 auto n=std::filesystem::file_size(path);if(n>65536)throw std::invalid_argument("Scene cache header budget");auto bytes=read(path,static_cast<std::size_t>(n));if(picosha2::hash256_hex_string(bytes)!=expected->second)throw std::runtime_error("Immutable scene snapshot changed; restart analysis context");auto j=Json::parse(bytes);
 if((j.at("version")!=1&&j.at("version")!=2)||j.at("key")!=key||j.at("policy")!=impl_->policy||j.at("width")!=i.width||j.at("height")!=i.height||j.at("source_height").get<double>()!=height)throw std::runtime_error("Scene cache identity mismatch");
 const std::size_t pixels=std::size_t(i.width)*std::size_t(i.height);const auto&people=j.at("people");if(!people.is_array()||people.size()>16)throw std::invalid_argument("Scene cache person budget");
 std::size_t budget=pixels*sizeof(SceneWeights);for(const auto&p:people){auto samples=p.at("samples").get<std::size_t>();if(samples>32768)throw std::invalid_argument("Scene sample budget");budget+=pixels*4+samples*sizeof(SkinSample);if(p.contains("toning_support_sha256"))budget+=pixels*4;if(budget>max_entry)throw std::invalid_argument("Scene entry budget");}
 if(budget>max_entry)throw std::invalid_argument("Scene entry budget");
 auto result=std::make_shared<SceneAnalysis>();result->width=i.width;result->height=i.height;
 auto weights=payload(impl_->directory/(key+".weights"),pixels*8,j.at("scene_sha256"));result->scene.resize(pixels);
 for(std::size_t p=0;p<pixels;++p){std::copy_n(weights.begin()+static_cast<std::ptrdiff_t>(p*8),8,result->scene[p].begin());validate_scene_weights(result->scene[p]);}
 weights.clear();weights.shrink_to_fit();std::vector<double> mass(pixels);
 for(std::size_t k=0;k<people.size();++k){const auto stem=key+"-p"+std::to_string(k);PersonAnalysis person;if(people[k].contains("toning_support_sha256")){if(j.at("version")!=2)throw std::invalid_argument("Stable field needs schema 2");person.toning_support=payload(impl_->directory/(stem+".support"),pixels,people[k].at("toning_support_sha256"));person.toning_strength=people[k].at("toning_strength").get<double>();if(!std::isfinite(person.toning_strength)||person.toning_strength<0||person.toning_strength>1)throw std::invalid_argument("Invalid stable strength");for(float value:person.toning_support)unit(value);}person.mask=payload(impl_->directory/(stem+".mask"),pixels,people[k].at("mask_sha256"));auto samples=payload(impl_->directory/(stem+".samples"),people[k].at("samples").get<std::size_t>()*4,people[k].at("samples_sha256"));
  for(std::size_t p=0;p<pixels;++p){unit(person.mask[p]);mass[p]+=person.mask[p];if(mass[p]>result->scene[p][0]+1e-6)throw std::invalid_argument("Cached people exceed skin budget");}
  person.samples.reserve(samples.size()/4);for(std::size_t p=0;p<samples.size();p+=4){unit(samples[p+3]);for(std::size_t c=0;c<3;++c)unit(samples[p+c]);person.samples.push_back({{samples[p],samples[p+1],samples[p+2]},samples[p+3]});}result->people.push_back(std::move(person));
 }
 if(impl_->trace)std::fprintf(stderr,"FSTR_SCENE DISK %s\n",key.c_str());
 impl_->entries.push_front({key,result,budget});impl_->bytes+=budget;while(impl_->entries.size()>4||impl_->bytes>max_resident){impl_->bytes-=impl_->entries.back().bytes;impl_->entries.pop_back();}return result;
}
}
