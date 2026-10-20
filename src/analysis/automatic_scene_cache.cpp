#include <condition_variable>
#include <map>
#include "scene_payload_codec.hpp"
#include "fstr/automatic_scene_cache.hpp"
#include "json.hpp"
#include "picosha2.h"
#include "fstr/byte_sha256.hpp"
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
#include <cstdint>
#if defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace fstr {namespace {
constexpr std::size_t max_entry=64*1024*1024,max_resident=128*1024*1024;
// Byte budget controls real mask storage; the count only bounds tiny-entry overhead.
constexpr std::size_t max_disk_entries=4096;
// One stable inode coordinates all cooperating writers/recovery in this root.
// Never unlink the lock file: that would allow simultaneous locks on two inodes.
class RootWriteLock final {
public:
 explicit RootWriteLock(const std::filesystem::path&root){
#if defined(__APPLE__) || defined(__linux__)
  fd_=::open((root/".fstr-write-v1.lock").c_str(),O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
  if(fd_<0)return;struct stat status{};
  if(::fstat(fd_,&status)!=0||!S_ISREG(status.st_mode)||::flock(fd_,LOCK_EX|LOCK_NB)!=0){::close(fd_);fd_=-1;}
#else
  (void)root; // Prior persistence retained; no recovery on unqualified platforms.
#endif
 }
 ~RootWriteLock(){
#if defined(__APPLE__) || defined(__linux__)
  if(fd_>=0)::close(fd_);
#endif
 }
 RootWriteLock(const RootWriteLock&)=delete;
 RootWriteLock&operator=(const RootWriteLock&)=delete;
 explicit operator bool()const noexcept{
#if defined(__APPLE__) || defined(__linux__)
  return fd_>=0;
#else
  return true;
#endif
 }
private:
#if defined(__APPLE__) || defined(__linux__)
 int fd_=-1;
#endif
};
bool managed_temporary(const std::string&name){
 const std::string prefix=".building-v1-";if(name.compare(0,prefix.size(),prefix)!=0)return false;
 auto separator=name.find('-',prefix.size());if(separator==std::string::npos)return false;
 auto number=[](const std::string&s){if(s.empty()||s.size()>10||(s.size()>1&&s[0]=='0'))return false;std::uint64_t value=0;for(char c:s){if(c<'0'||c>'9')return false;value=value*10+unsigned(c-'0');}return value<=UINT32_MAX;};
 return number(name.substr(prefix.size(),separator-prefix.size()))&&number(name.substr(separator+1));
}
// Caller owns RootWriteLock. Legacy writers lack this protocol and are excluded.
void remove_abandoned(const std::filesystem::path&root){
#if defined(__APPLE__) || defined(__linux__)
 for(const auto&e:std::filesystem::directory_iterator(root))if(managed_temporary(e.path().filename().string())&&std::filesystem::is_directory(e.symlink_status()))std::filesystem::remove_all(e.path());
#else
 (void)root;
#endif
}
void recover(const std::filesystem::path&root)noexcept{
 try{if(!std::filesystem::is_directory(std::filesystem::symlink_status(root)))return;RootWriteLock lock(root);if(lock)remove_abandoned(root);}catch(const std::exception&){/* Optional maintenance cannot disable analysis. */}
}
std::size_t validate(const SceneAnalysis&a,const AnalysisImage&i){
 if(a.width!=i.width||a.height!=i.height||a.scene.size()!=std::size_t(i.width)*std::size_t(i.height)||a.people.size()>16)throw std::invalid_argument("Automatic scene geometry mismatch");std::size_t bytes=a.scene.size()*sizeof(SceneWeights);for(const auto&w:a.scene)validate_scene_weights(w);std::vector<double> mass(a.scene.size());
 for(const auto&p:a.people){if(p.mask.size()!=a.scene.size()||(!p.toning_support.empty()&&p.toning_support.size()!=a.scene.size())||p.samples.size()>32768)throw std::invalid_argument("Automatic person extent mismatch");auto unit=[](double v){if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Automatic scene confidence invalid");};unit(p.toning_strength);unit(p.detection_confidence);for(double v:p.box)unit(v);for(double v:p.descriptor)if(!std::isfinite(v)||v<0||v>16)throw std::invalid_argument("Automatic person descriptor invalid");
  for(std::size_t j=0;j<a.scene.size();++j){unit(p.mask[j]);mass[j]+=p.mask[j];if(mass[j]>a.scene[j][0]+1e-6)throw std::invalid_argument("Automatic people exceed skin budget");}for(float v:p.toning_support)unit(v);for(const auto&s:p.samples){unit(s.color.r);unit(s.color.g);unit(s.color.b);unit(s.confidence);}bytes+=(p.mask.size()+p.toning_support.size())*4+p.samples.size()*sizeof(SkinSample);if(bytes>max_entry)throw std::invalid_argument("Automatic scene byte budget");
 }if(bytes>max_entry)throw std::invalid_argument("Automatic scene byte budget");return bytes;
}
std::string floats(const float* data,std::size_t n){if(n>max_entry/4)throw std::invalid_argument("Automatic payload budget");std::string bytes(n*4,'\0');for(std::size_t i=0;i<n;++i){std::uint32_t value;std::memcpy(&value,data+i,4);for(std::size_t c=0;c<4;++c)bytes[i*4+c]=char((value>>(c*8))&255);}return bytes;}
void write(const std::filesystem::path&p,const std::string&bytes){std::ofstream f(p,std::ios::binary);f.write(bytes.data(),std::streamsize(bytes.size()));f.close();if(!f)throw std::runtime_error("Automatic cache write failed");}
std::string payload(const std::filesystem::path&p,const float*data,std::size_t n){auto bytes=floats(data,n);write(p,cache_codec::encode(bytes));return byte_sha256(bytes);}
bool key_text(const std::string&s){return s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
void store(const std::filesystem::path&root,const std::string&key,const std::string&policy,const SceneAnalysis&a,double height,const SceneDiskBudget& budget){
 if(!budget.bytes)return;
 std::filesystem::create_directories(root);RootWriteLock lock(root);if(!lock)return;
 // Best-effort reserve: other applications can consume disk concurrently.
 // Budget checks are advisory persistence only; valid analysis remains usable.
 const auto available=std::filesystem::space(root).available;
 if(available==std::uintmax_t(-1)||available<budget.free_reserve||available-budget.free_reserve<max_entry+2*1024*1024)return;
 remove_abandoned(root);
 auto destination=root/(key+".entry");if(std::filesystem::exists(std::filesystem::symlink_status(destination)))std::filesystem::remove_all(destination);
#if defined(__APPLE__) || defined(__linux__)
 const std::string prefix=".building-v1-";
#else
 const std::string prefix=".building-";
#endif
 std::filesystem::path temporary_path;std::random_device random;for(int attempt=0;attempt<32;++attempt){auto candidate=root/(prefix+std::to_string(random())+"-"+std::to_string(random()));if(std::filesystem::create_directory(candidate)){temporary_path=std::move(candidate);break;}}if(temporary_path.empty())throw std::runtime_error("Automatic cache temporary failed");struct Temporary{std::filesystem::path path;~Temporary(){std::error_code e;std::filesystem::remove_all(path,e);}} temporary{temporary_path};const auto dir=temporary.path;
 nlohmann::json j{{"version",4},{"payload_codec","zlib-1"},{"key",key},{"policy",policy},{"width",a.width},{"height",a.height},{"source_height",height},{"people",nlohmann::json::array()}};std::vector<float> scene;scene.reserve(a.scene.size()*8);for(const auto&w:a.scene)scene.insert(scene.end(),w.begin(),w.end());j["scene_sha256"]=payload(dir/(key+".weights"),scene.data(),scene.size());
 for(std::size_t i=0;i<a.people.size();++i){const auto&p=a.people[i];auto stem=key+"-p"+std::to_string(i);std::vector<float> samples;samples.reserve(p.samples.size()*4);for(const auto&s:p.samples)for(double v:{s.color.r,s.color.g,s.color.b,s.confidence})samples.push_back(float(v));nlohmann::json item{{"samples",p.samples.size()},{"samples_sha256",payload(dir/(stem+".samples"),samples.data(),samples.size())},{"mask_sha256",payload(dir/(stem+".mask"),p.mask.data(),p.mask.size())},{"box",p.box},{"descriptor",p.descriptor},{"detection_confidence",p.detection_confidence}};
  if(!p.toning_support.empty()){item["toning_support_sha256"]=payload(dir/(stem+".support"),p.toning_support.data(),p.toning_support.size());item["toning_strength"]=p.toning_strength;}j["people"].push_back(std::move(item));
 }auto header=j.dump();if(header.size()>65536)throw std::invalid_argument("Automatic cache header budget");write(dir/(key+".fstrscene"),header);
 // A complete entry directory appears atomically. Never overwrite an active
 // immutable entry with a partially published collection of payloads.
 std::uintmax_t entry_bytes=0;for(const auto& part:std::filesystem::directory_iterator(dir))if(std::filesystem::is_regular_file(part.symlink_status()))entry_bytes+=part.file_size();
 if(entry_bytes>budget.bytes)return; // Temporary RAII cleanup; valid analysis remains resident.
 std::error_code error;std::filesystem::rename(dir,destination,error);if(error){if(!std::filesystem::is_directory(std::filesystem::symlink_status(destination)))throw std::runtime_error("Automatic cache publish failed");}
 std::vector<std::pair<std::filesystem::directory_entry,std::uintmax_t>> entries;std::uintmax_t total=0;
 for(const auto&e:std::filesystem::directory_iterator(root)){auto filename=e.path().filename().string();if(filename.size()!=70||filename.substr(64)!=".entry"||!key_text(filename.substr(0,64))||!std::filesystem::is_directory(e.symlink_status()))continue;std::uintmax_t bytes=0;for(const auto&p:std::filesystem::directory_iterator(e.path()))if(std::filesystem::is_regular_file(p.symlink_status()))bytes+=p.file_size();entries.push_back({e,bytes});total+=bytes;}
 std::sort(entries.begin(),entries.end(),[](const auto&a,const auto&b){return a.first.last_write_time()<b.first.last_write_time();});std::size_t remaining=entries.size();for(std::size_t k=0;k<entries.size()&&(total>budget.bytes||remaining>max_disk_entries);++k){if(entries[k].first.path()==destination)continue;std::filesystem::remove_all(entries[k].first.path());total-=entries[k].second;--remaining;}
}
}
namespace {
// A recursive miss from compute/cancellation must fail instead of waiting on
// this thread's own serialized backend. Resident hits remain safe to borrow.
struct ActiveSceneMiss {
 const void* cache;ActiveSceneMiss* previous;
 static thread_local ActiveSceneMiss* current;
 static bool contains(const void* cache)noexcept{for(auto p=current;p;p=p->previous)if(p->cache==cache)return true;return false;}
 explicit ActiveSceneMiss(const void* c)noexcept:cache(c),previous(current){current=this;}
 ~ActiveSceneMiss(){current=previous;}
};
thread_local ActiveSceneMiss* ActiveSceneMiss::current=nullptr;
}
namespace {
// Separate bounded lanes keep saved-mask reads independent of model work.
// A lease never invokes caller code or cancellation while holding its mutex.
class WorkLane {
 std::mutex mutex;std::condition_variable changed;unsigned active=0,limit;
public:
 explicit WorkLane(unsigned n):limit(n){}
 class Lease {
  WorkLane* lane;
 public:
  Lease(WorkLane& value,const std::function<void()>& abort):lane(&value){
   for(;;){abort();std::unique_lock<std::mutex> lock(lane->mutex);if(lane->active<lane->limit){++lane->active;return;}lane->changed.wait_for(lock,std::chrono::milliseconds(2));}
  }
  ~Lease(){ {std::lock_guard<std::mutex> lock(lane->mutex);--lane->active;}lane->changed.notify_one(); }
  Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
 };
};
}
struct AutomaticSceneCache::Impl {
 std::filesystem::path directory;std::string policy;SceneCompute compute;SceneDiskBudget budget;
 std::mutex resident;std::size_t generation=0;
 WorkLane reads{2},backend,writes{1};
 struct Entry{std::string key;std::shared_ptr<const SceneAnalysis> analysis;std::size_t bytes;};
 std::list<Entry> entries;std::size_t bytes=0;
 struct Flight{bool done=false;std::shared_ptr<const SceneAnalysis> result;std::condition_variable changed;};
 using FlightKey=std::pair<std::string,std::size_t>;
 std::map<FlightKey,std::shared_ptr<Flight>> flights; // Protected by resident.
 Impl(std::string d,std::string p,SceneCompute c,SceneDiskBudget b,unsigned count):directory(std::move(d)),policy(std::move(p)),compute(std::move(c)),budget(b),backend(count){if(count<1||count>2)throw std::invalid_argument("Automatic compute limit must be1or2");if(budget.bytes && (budget.bytes<max_entry || budget.bytes>32ULL*1024*1024*1024))throw std::invalid_argument("Automatic cache disk budget must be64MiB..32GiB or zero");if(!directory.is_absolute()||!key_text(policy)||!compute)throw std::invalid_argument("Invalid automatic cache configuration");recover(directory);}
};
AutomaticSceneCache::AutomaticSceneCache(std::string d,std::string p,SceneCompute c,SceneDiskBudget b,unsigned count):impl_(new Impl(std::move(d),std::move(p),std::move(c),b,count)){}
AutomaticSceneCache::~AutomaticSceneCache()=default;
const std::string& AutomaticSceneCache::guid()const noexcept{return impl_->policy;}
void AutomaticSceneCache::clear_memory(){
 std::list<Impl::Entry> retired;
 {std::lock_guard<std::mutex> lock(impl_->resident);++impl_->generation;retired.swap(impl_->entries);impl_->bytes=0;}
 // Release potentially large analyses after unlocking resident access.
}
std::shared_ptr<const SceneAnalysis> AutomaticSceneCache::get(const AnalysisImage&i,double height,const std::function<void()>&cancel,const std::string&source_identity){
 const bool trace=std::getenv("FSTR_LUT_SCENE_TRACE")&&std::string(std::getenv("FSTR_LUT_SCENE_TRACE"))=="1";
 if(!source_identity.empty()&&!key_text(source_identity))throw std::invalid_argument("Invalid upstream identity");
 const auto policy=source_identity.empty()?impl_->policy:picosha2::hash256_hex_string("FSTR_SOURCE_IDENTITY_V1\n"+impl_->policy+"\n"+source_identity);
 const auto key=scene_cache_key(i,height,policy);auto abort=[&]{if(cancel)cancel();};abort();
 std::size_t generation=0;
 auto resident=[&](bool remember_generation){
  std::lock_guard<std::mutex> lock(impl_->resident);if(remember_generation)generation=impl_->generation;
  auto found=std::find_if(impl_->entries.begin(),impl_->entries.end(),[&](const auto&e){return e.key==key;});
  if(found==impl_->entries.end())return std::shared_ptr<const SceneAnalysis>{};
  auto value=found->analysis;impl_->entries.splice(impl_->entries.begin(),impl_->entries,found);return value;
 };
 auto result=resident(true);
 auto return_resident=[&]{abort();if(trace)std::fprintf(stderr,"FSTR_AUTO RAM %s\n",key.c_str());return result;};
 if(result)return return_resident();
 if(ActiveSceneMiss::contains(impl_.get()))throw std::logic_error("Recursive automatic scene cache miss");
 ActiveSceneMiss active(impl_.get());
 const Impl::FlightKey flight_key{key,generation};std::shared_ptr<Impl::Flight> flight;
 for(;;){
  abort();result=resident(false);if(result)return return_resident();
  bool owner=false;
  {std::lock_guard<std::mutex> lock(impl_->resident);auto found=impl_->flights.find(flight_key);if(found!=impl_->flights.end())flight=found->second;else{flight=std::make_shared<Impl::Flight>();impl_->flights.emplace(flight_key,flight);owner=true;}}
  if(owner)break;
  for(;;){
   abort();std::unique_lock<std::mutex> lock(impl_->resident);
   if(flight->done){result=flight->result;break;}
   flight->changed.wait_for(lock,std::chrono::milliseconds(2));
  }
  if(result){abort();if(trace)std::fprintf(stderr,"FSTR_AUTO JOIN %s\n",key.c_str());return result;}
  // A failed/cancelled owner is not a cached error: waiters may retry with
  // their own cancellation while retaining their original clear generation.
 }
 auto finish=[&](std::shared_ptr<const SceneAnalysis> value){
  {std::lock_guard<std::mutex> lock(impl_->resident);flight->result=std::move(value);flight->done=true;impl_->flights.erase(flight_key);}
  flight->changed.notify_all();
 };
 try{
  // Recheck after becoming owner: a preceding owner may just have published.
  result=resident(false);
  if(result){abort();if(trace)std::fprintf(stderr,"FSTR_AUTO RAM %s\n",key.c_str());finish(result);return result;}
  if(!result){WorkLane::Lease read(impl_->reads,abort);auto directory=impl_->directory/(key+".entry");
   try{if(std::filesystem::is_directory(std::filesystem::symlink_status(directory))){SceneCache cache(directory.string(),policy);result=cache.get(i,height);if(result){validate(*result,i);if(trace)std::fprintf(stderr,"FSTR_AUTO DISK %s\n",key.c_str());}}}catch(const std::exception&){result.reset();}
  }
  abort();if(!result){
   WorkLane::Lease compute(impl_->backend,abort);abort();if(trace)std::fprintf(stderr,"FSTR_AUTO COMPUTE %s\n",key.c_str());result=impl_->compute(i,height,cancel);abort();
   if(!result)throw std::runtime_error("Automatic scene analysis returned no result");validate(*result,i);
   // Within one context, avoid dropping persistence because a peer owns the
   // filesystem writer. The existing cross-process root lock stays intact.
   WorkLane::Lease write(impl_->writes,abort);
   try{store(impl_->directory,key,policy,*result,height,impl_->budget);}catch(const std::exception&){/* Optional persistence; keep valid current analysis. */}
  }
  const auto bytes=validate(*result,i);abort();std::list<Impl::Entry> retired;
  {std::lock_guard<std::mutex> lock(impl_->resident);
   if(generation==impl_->generation){
    impl_->entries.push_front({key,result,bytes});impl_->bytes+=bytes;
    while(impl_->entries.size()>8||impl_->bytes>max_resident){impl_->bytes-=impl_->entries.back().bytes;retired.splice(retired.begin(),impl_->entries,std::prev(impl_->entries.end()));}
   }
  }
  finish(result);
 }catch(...){finish({});throw;}

 return result;
}
}
