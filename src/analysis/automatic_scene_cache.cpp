#include "fstr/automatic_scene_cache.hpp"
#include "json.hpp"
#include "picosha2.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <list>
#include <mutex>
#include <thread>
#include <random>
#include <cstdlib>
#include <cstdio>
namespace fstr {namespace {
constexpr std::size_t max_entry=64*1024*1024,max_resident=128*1024*1024,max_disk=512*1024*1024;
std::size_t validate(const SceneAnalysis&a,const AnalysisImage&i){
 if(a.width!=i.width||a.height!=i.height||a.scene.size()!=std::size_t(i.width)*std::size_t(i.height)||a.people.size()>16)throw std::invalid_argument("Automatic scene geometry mismatch");std::size_t bytes=a.scene.size()*sizeof(SceneWeights);for(const auto&w:a.scene)validate_scene_weights(w);std::vector<double> mass(a.scene.size());
 for(const auto&p:a.people){if(p.mask.size()!=a.scene.size()||(!p.toning_support.empty()&&p.toning_support.size()!=a.scene.size())||p.samples.size()>32768)throw std::invalid_argument("Automatic person extent mismatch");auto unit=[](double v){if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Automatic scene confidence invalid");};unit(p.toning_strength);unit(p.detection_confidence);for(double v:p.box)unit(v);for(double v:p.descriptor)if(!std::isfinite(v)||v<0||v>16)throw std::invalid_argument("Automatic person descriptor invalid");
  for(std::size_t j=0;j<a.scene.size();++j){unit(p.mask[j]);mass[j]+=p.mask[j];if(mass[j]>a.scene[j][0]+1e-6)throw std::invalid_argument("Automatic people exceed skin budget");}for(float v:p.toning_support)unit(v);for(const auto&s:p.samples){unit(s.color.r);unit(s.color.g);unit(s.color.b);unit(s.confidence);}bytes+=(p.mask.size()+p.toning_support.size())*4+p.samples.size()*sizeof(SkinSample);if(bytes>max_entry)throw std::invalid_argument("Automatic scene byte budget");
 }if(bytes>max_entry)throw std::invalid_argument("Automatic scene byte budget");return bytes;
}
std::string floats(const float* data,std::size_t n){if(n>max_entry/4)throw std::invalid_argument("Automatic payload budget");std::string bytes(n*4,'\0');for(std::size_t i=0;i<n;++i){std::uint32_t value;std::memcpy(&value,data+i,4);for(std::size_t c=0;c<4;++c)bytes[i*4+c]=char((value>>(c*8))&255);}return bytes;}
void write(const std::filesystem::path&p,const std::string&bytes){std::ofstream f(p,std::ios::binary);f.write(bytes.data(),std::streamsize(bytes.size()));f.close();if(!f)throw std::runtime_error("Automatic cache write failed");}
std::string payload(const std::filesystem::path&p,const float*data,std::size_t n){auto bytes=floats(data,n);write(p,bytes);return picosha2::hash256_hex_string(bytes);}
bool key_text(const std::string&s){return s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
void store(const std::filesystem::path&root,const std::string&key,const std::string&policy,const SceneAnalysis&a,double height){
 std::filesystem::create_directories(root);std::filesystem::path temporary_path;std::random_device random;for(int attempt=0;attempt<32;++attempt){auto candidate=root/(".building-"+std::to_string(random())+"-"+std::to_string(random()));if(std::filesystem::create_directory(candidate)){temporary_path=std::move(candidate);break;}}if(temporary_path.empty())throw std::runtime_error("Automatic cache temporary failed");struct Temporary{std::filesystem::path path;~Temporary(){std::error_code e;std::filesystem::remove_all(path,e);}} temporary{temporary_path};const auto dir=temporary.path;
 nlohmann::json j{{"version",3},{"key",key},{"policy",policy},{"width",a.width},{"height",a.height},{"source_height",height},{"people",nlohmann::json::array()}};std::vector<float> scene;scene.reserve(a.scene.size()*8);for(const auto&w:a.scene)scene.insert(scene.end(),w.begin(),w.end());j["scene_sha256"]=payload(dir/(key+".weights"),scene.data(),scene.size());
 for(std::size_t i=0;i<a.people.size();++i){const auto&p=a.people[i];auto stem=key+"-p"+std::to_string(i);std::vector<float> samples;samples.reserve(p.samples.size()*4);for(const auto&s:p.samples)for(double v:{s.color.r,s.color.g,s.color.b,s.confidence})samples.push_back(float(v));nlohmann::json item{{"samples",p.samples.size()},{"samples_sha256",payload(dir/(stem+".samples"),samples.data(),samples.size())},{"mask_sha256",payload(dir/(stem+".mask"),p.mask.data(),p.mask.size())},{"box",p.box},{"descriptor",p.descriptor},{"detection_confidence",p.detection_confidence}};
  if(!p.toning_support.empty()){item["toning_support_sha256"]=payload(dir/(stem+".support"),p.toning_support.data(),p.toning_support.size());item["toning_strength"]=p.toning_strength;}j["people"].push_back(std::move(item));
 }auto header=j.dump();if(header.size()>65536)throw std::invalid_argument("Automatic cache header budget");write(dir/(key+".fstrscene"),header);
 // A complete entry directory appears atomically. Never overwrite an active
 // immutable entry with a partially published collection of payloads.
 auto destination=root/(key+".entry");std::error_code error;std::filesystem::rename(dir,destination,error);if(error){if(!std::filesystem::is_directory(std::filesystem::symlink_status(destination)))throw std::runtime_error("Automatic cache publish failed");}
 std::vector<std::pair<std::filesystem::directory_entry,std::uintmax_t>> entries;std::uintmax_t total=0;
 for(const auto&e:std::filesystem::directory_iterator(root)){auto filename=e.path().filename().string();if(filename.size()!=70||filename.substr(64)!=".entry"||!key_text(filename.substr(0,64))||!std::filesystem::is_directory(e.symlink_status()))continue;std::uintmax_t bytes=0;for(const auto&p:std::filesystem::directory_iterator(e.path()))if(std::filesystem::is_regular_file(p.symlink_status()))bytes+=p.file_size();entries.push_back({e,bytes});total+=bytes;}
 std::sort(entries.begin(),entries.end(),[](const auto&a,const auto&b){return a.first.last_write_time()<b.first.last_write_time();});std::size_t remaining=entries.size();for(std::size_t k=0;k<entries.size()&&(total>max_disk||remaining>64);++k){if(entries[k].first.path()==destination)continue;std::filesystem::remove_all(entries[k].first.path());total-=entries[k].second;--remaining;}
}
}
struct AutomaticSceneCache::Impl {std::filesystem::path directory;std::string policy;SceneCompute compute;std::mutex mutex;struct Entry{std::string key;std::shared_ptr<const SceneAnalysis> analysis;std::size_t bytes;};std::list<Entry> entries;std::size_t bytes=0;
 Impl(std::string d,std::string p,SceneCompute c):directory(std::move(d)),policy(std::move(p)),compute(std::move(c)){if(!directory.is_absolute()||!key_text(policy)||!compute)throw std::invalid_argument("Invalid automatic cache configuration");}
};
AutomaticSceneCache::AutomaticSceneCache(std::string d,std::string p,SceneCompute c):impl_(new Impl(std::move(d),std::move(p),std::move(c))){}
AutomaticSceneCache::~AutomaticSceneCache()=default;
const std::string& AutomaticSceneCache::guid()const noexcept{return impl_->policy;}
void AutomaticSceneCache::clear_memory(){std::lock_guard<std::mutex> lock(impl_->mutex);impl_->entries.clear();impl_->bytes=0;}
std::shared_ptr<const SceneAnalysis> AutomaticSceneCache::get(const AnalysisImage&i,double height,const std::function<void()>&cancel,const std::string&source_identity){
 const bool trace=std::getenv("FSTR_LUT_SCENE_TRACE")&&std::string(std::getenv("FSTR_LUT_SCENE_TRACE"))=="1";if(!source_identity.empty()&&!key_text(source_identity))throw std::invalid_argument("Invalid upstream identity");const auto policy=source_identity.empty()?impl_->policy:picosha2::hash256_hex_string("FSTR_SOURCE_IDENTITY_V1\n"+impl_->policy+"\n"+source_identity);auto key=scene_cache_key(i,height,policy);auto abort=[&]{if(cancel)cancel();};abort();std::unique_lock<std::mutex> lock(impl_->mutex,std::defer_lock);while(!lock.try_lock()){abort();std::this_thread::sleep_for(std::chrono::milliseconds(2));}abort();auto found=std::find_if(impl_->entries.begin(),impl_->entries.end(),[&](const auto&e){return e.key==key;});if(found!=impl_->entries.end()){if(trace)std::fprintf(stderr,"FSTR_AUTO RAM %s\n",key.c_str());auto value=found->analysis;impl_->entries.splice(impl_->entries.begin(),impl_->entries,found);return value;}
 std::shared_ptr<const SceneAnalysis> result;auto directory=impl_->directory/(key+".entry");try{if(std::filesystem::is_directory(std::filesystem::symlink_status(directory))){SceneCache cache(directory.string(),policy);result=cache.get(i,height);if(result){validate(*result,i);if(trace)std::fprintf(stderr,"FSTR_AUTO DISK %s\n",key.c_str());}}}catch(const std::exception&){result.reset();/* Invalid data is a miss; no false analysis success. */}
 abort();if(!result){if(trace)std::fprintf(stderr,"FSTR_AUTO COMPUTE %s\n",key.c_str());result=impl_->compute(i,height,cancel);abort();if(!result)throw std::runtime_error("Automatic scene analysis returned no result");validate(*result,i);try{if(std::filesystem::exists(std::filesystem::symlink_status(directory)))std::filesystem::remove_all(directory);store(impl_->directory,key,policy,*result,height);}catch(const std::exception&){/* Persistence failure retains valid current analysis. */}}
 auto bytes=validate(*result,i);impl_->entries.push_front({key,result,bytes});impl_->bytes+=bytes;while(impl_->entries.size()>4||impl_->bytes>max_resident){impl_->bytes-=impl_->entries.back().bytes;impl_->entries.pop_back();}return result;
}
}
