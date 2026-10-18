#include "fstr/sdr_prepare_cache.hpp"
#include "json.hpp"
#include "picosha2.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cerrno>
#include <filesystem>
#include <fstream>
#include <list>
#include <mutex>
#include <random>
#include <regex>
#include <thread>
#include <cstdlib>
#include <cstdio>
#if defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace fstr { namespace {
using Json=nlohmann::json;
constexpr std::size_t entry_budget=4096,disk_budget=8*1024*1024,disk_count=2048;
std::string hash(const std::string&s){return picosha2::hash256_hex_string(s);}
bool hex(const std::string&s){return s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
bool managed(const std::filesystem::path&p){const auto s=p.filename().string();return s.size()==73&&s.substr(64)==".fstrprep"&&hex(s.substr(0,64));}
std::string key(const std::vector<FrameView>&frames,const std::string&metadata){
    if(frames.empty()||frames.size()>16||metadata.size()>2048)throw std::invalid_argument("Invalid preparation cache context");
    std::vector<std::string> parts;
    for(auto f:frames){
        if(!f.data||!f.width||!f.height||f.width>96||f.height>54||f.row_stride<f.width*4||f.row_stride>100000||f.count<(f.height-1)*f.row_stride+f.width*4||f.memory!=Memory::CPU)throw std::invalid_argument("Invalid preparation snapshot");
        std::string bytes=std::to_string(f.width)+"/"+std::to_string(f.height)+"/"+std::to_string(int(f.transfer))+"/"+std::to_string(int(f.primaries))+"/"+std::to_string(int(f.alpha))+"\n";
        for(std::size_t y=0;y<f.height;++y)for(std::size_t x=0;x<f.width*4;++x){std::uint32_t v=0;std::memcpy(&v,f.data+y*f.row_stride+x,4);for(int b=0;b<4;++b)bytes.push_back(char((v>>(b*8))&255));}
        parts.push_back(hash(bytes));
    }
    std::sort(parts.begin(),parts.end());std::string all="FSTR_SDR_SHOT_V2/CLIP_CACHE_V1/" FSTR_SDR_PREPARATION_ID "\n"+metadata+"\n";for(const auto&p:parts)all+=p+"\n";return hash(all);
}
std::string payload(const SdrShotPreparation& s){validate_sdr_preparation(s.settings);return Json{{"decision",int(s.decision)},{"contrast",s.settings.contrast},{"bias",s.settings.brightness_bias},{"saturation",s.settings.saturation},{"amount",s.settings.amount},{"low",s.low},{"median",s.median},{"high",s.high},{"flatness",s.flatness},{"samples",s.samples}}.dump();}
SdrShotPreparation decode(const std::string&text){
    const auto j=Json::parse(text);SdrShotPreparation s;int decision=j.at("decision");if(decision<0||decision>3)throw std::invalid_argument("Invalid preparation decision");s.decision=static_cast<SdrDecision>(decision);s.settings={j.at("contrast"),j.at("bias"),j.at("saturation"),j.at("amount")};validate_sdr_preparation(s.settings);s.low=j.at("low");s.median=j.at("median");s.high=j.at("high");s.flatness=j.at("flatness");s.samples=j.at("samples");for(double v:{s.low,s.median,s.high,s.flatness})if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Invalid preparation statistics");if(s.samples>16*96*54)throw std::invalid_argument("Preparation sample budget");return s;
}
class RootLock final {
public:
    explicit RootLock(const std::filesystem::path& root){
#if defined(__APPLE__) || defined(__linux__)
        fd_=::open((root/".write-v1.lock").c_str(),O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);struct stat status{};
        if(fd_>=0&&(::fstat(fd_,&status)!=0||!S_ISREG(status.st_mode)||::flock(fd_,LOCK_EX|LOCK_NB)!=0)){::close(fd_);fd_=-1;}
#else
        (void)root;
#endif
    }
    ~RootLock(){
#if defined(__APPLE__) || defined(__linux__)
        if(fd_>=0)::close(fd_);
#endif
    }
    explicit operator bool()const noexcept{return fd_>=0;}
    RootLock(const RootLock&)=delete;RootLock&operator=(const RootLock&)=delete;
private:int fd_=-1;
};
bool pending(const std::string& name){
    static const std::regex pattern("^\\.pending-v1-[0-9a-f]{64}-([0-9]{1,10})-([0-9]{1,10})$");std::smatch match;if(!std::regex_match(name,match,pattern))return false;
    for(std::size_t i=1;i<=2;++i){const auto n=match[i].str();if((n.size()>1&&n[0]=='0')||std::stoull(n)>UINT32_MAX)return false;}return true;
}
void clean_pending(const std::filesystem::path&root){for(const auto&e:std::filesystem::directory_iterator(root))if(std::filesystem::is_regular_file(e.symlink_status())&&pending(e.path().filename().string()))std::filesystem::remove(e.path());}
void recover(const std::filesystem::path&root)noexcept{try{if(root.empty()||!std::filesystem::is_directory(std::filesystem::symlink_status(root)))return;RootLock lock(root);if(lock)clean_pending(root);}catch(const std::exception&) {}}
void store(const std::filesystem::path&root,const std::string&k,const SdrShotPreparation&s){
#if defined(__APPLE__) || defined(__linux__)
    if(root.empty())return;std::filesystem::create_directories(root);if(!std::filesystem::is_directory(std::filesystem::symlink_status(root)))return;
    RootLock lock(root);if(!lock)return;clean_pending(root);
    const auto p=payload(s),data=Json{{"version",1},{"key",k},{"sha256",hash(p)},{"payload",p}}.dump();if(data.size()>entry_budget)throw std::invalid_argument("Preparation entry budget");
    std::random_device random;const auto temporary=root/(".pending-v1-"+k+"-"+std::to_string(random())+"-"+std::to_string(random()));
    struct File{int fd=-1;std::filesystem::path path;~File(){if(fd>=0)::close(fd);std::error_code e;if(!path.empty())std::filesystem::remove(path,e);}} file;
    file.path=temporary;file.fd=::open(temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);if(file.fd<0)throw std::runtime_error("Preparation cache temporary failed");
    std::size_t written=0;while(written<data.size()){const auto n=::write(file.fd,data.data()+written,data.size()-written);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("Preparation cache write failed");written+=std::size_t(n);}
    if(::close(file.fd)!=0){file.fd=-1;throw std::runtime_error("Preparation cache close failed");}file.fd=-1;const auto destination=root/(k+".fstrprep");std::filesystem::rename(temporary,destination);
    std::vector<std::filesystem::directory_entry> entries;std::uintmax_t total=0;for(const auto&e:std::filesystem::directory_iterator(root))if(managed(e.path())&&std::filesystem::is_regular_file(e.symlink_status())){entries.push_back(e);total+=e.file_size();}
    std::sort(entries.begin(),entries.end(),[](const auto&a,const auto&b){return a.last_write_time()<b.last_write_time();});std::size_t count=entries.size();for(const auto&e:entries){if(count<=disk_count&&total<=disk_budget)break;if(e.path()==destination)continue;total-=e.file_size();std::filesystem::remove(e.path());--count;}
#else
    (void)root;(void)k;(void)s; // Unqualified platforms keep RAM analysis; no unsafe disk fallback.
#endif
}
}
std::vector<std::int32_t> sdr_clip_anchor_times(std::int32_t total,std::int32_t step){
    if(total<=0||step<=0)throw std::invalid_argument("Auto Prepare requires fixed clip time bounds");const std::int64_t last=(std::int64_t(total)-1)/step;std::vector<std::int32_t> result;for(auto f:{std::int64_t(0),last/2,last}){auto t=std::int32_t(f*step);if(result.empty()||result.back()!=t)result.push_back(t);}return result;
}
struct SdrPreparationCache::Impl{std::filesystem::path directory;std::mutex mutex;std::list<CachedSdrPreparation> memory;explicit Impl(std::string d):directory(std::move(d)){if(!directory.empty()&&!directory.is_absolute())throw std::invalid_argument("Preparation cache path must be absolute");recover(directory);}};
SdrPreparationCache::SdrPreparationCache(std::string d):impl_(new Impl(std::move(d))){}
SdrPreparationCache::~SdrPreparationCache()=default;
void SdrPreparationCache::clear_memory(){std::lock_guard<std::mutex> guard(impl_->mutex);impl_->memory.clear();}
CachedSdrPreparation SdrPreparationCache::get(const std::vector<FrameView>&frames,const std::string&metadata,const std::function<void()>&cancel){
    auto abort=[&]{if(cancel)cancel();};abort();const auto k=key(frames,metadata);abort();std::unique_lock<std::mutex> lock(impl_->mutex,std::defer_lock);while(!lock.try_lock()){abort();std::this_thread::sleep_for(std::chrono::milliseconds(2));}abort();
    const bool trace=std::getenv("FSTR_LUT_SCENE_TRACE")&&std::string(std::getenv("FSTR_LUT_SCENE_TRACE"))=="1";const auto found=std::find_if(impl_->memory.begin(),impl_->memory.end(),[&](const auto&e){return e.key==k;});if(found!=impl_->memory.end()){auto r=*found;r.use=SdrCacheUse::Memory;impl_->memory.splice(impl_->memory.begin(),impl_->memory,found);if(trace)std::fprintf(stderr,"FSTR_PREP RAM %s\n",k.c_str());return r;}
    CachedSdrPreparation result;result.key=k;bool hit=false;try{const auto p=impl_->directory/(k+".fstrprep");if(!impl_->directory.empty()&&std::filesystem::is_directory(std::filesystem::symlink_status(impl_->directory))&&std::filesystem::is_regular_file(std::filesystem::symlink_status(p))&&std::filesystem::file_size(p)<=entry_budget){std::ifstream f(p,std::ios::binary);const std::string text((std::istreambuf_iterator<char>(f)),{});const auto j=Json::parse(text);const auto body=j.at("payload").get<std::string>();if(j.at("version")==1&&j.at("key")==k&&j.at("sha256")==hash(body)){result.shot=decode(body);hit=true;}}}catch(const std::exception&){}
    abort();if(!hit){result.shot=analyze_sdr_shot(frames);abort();try{store(impl_->directory,k,result.shot);}catch(const std::exception&){};}
    result.use=hit?SdrCacheUse::Disk:SdrCacheUse::Computed;if(trace)std::fprintf(stderr,"FSTR_PREP %s %s\n",hit?"DISK":"COMPUTE",k.c_str());impl_->memory.push_front(result);while(impl_->memory.size()>64)impl_->memory.pop_back();return result;
}
}
