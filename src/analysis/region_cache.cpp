#include "fstr/region_cache.hpp"
#include "json.hpp"
#include "picosha2.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <list>
#include <locale>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <cerrno>
namespace fstr {
namespace {
constexpr std::size_t max_bytes=1024*1024;
using Json=nlohmann::json;
std::string digest(const std::string& text){return picosha2::hash256_hex_string(text);}
void validate(const std::vector<ColorRegion>& rs){
 if(rs.size()>64)throw std::invalid_argument("Too many cached regions");
 for(const auto&r:rs){
  for(double v:{r.x0,r.y0,r.x1,r.y1,r.detail})if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Invalid cached region");
  if(r.x1<=r.x0||r.y1<=r.y0||r.holes.size()>8)throw std::invalid_argument("Invalid cached geometry");
  for(const auto&h:r.holes){if(h.size()<3||h.size()>256)throw std::invalid_argument("Invalid cached polygon");for(const auto&p:h)for(double v:p)if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Invalid cached point");}
 }
}
std::string encode(const std::vector<ColorRegion>& rs){validate(rs);Json j=Json::array();for(const auto&r:rs)j.push_back({r.x0,r.y0,r.x1,r.y1,r.detail,r.holes});auto s=j.dump();if(s.size()>max_bytes/2)throw std::invalid_argument("Analysis cache entry too large");return s;}
std::vector<ColorRegion> decode(const std::string&s){auto j=Json::parse(s);if(!j.is_array()||j.size()>64)throw std::invalid_argument("Invalid analysis cache");std::vector<ColorRegion> rs;for(const auto&v:j){if(!v.is_array()||v.size()!=6)throw std::invalid_argument("Invalid region record");rs.push_back({v.at(0).get<double>(),v.at(1).get<double>(),v.at(2).get<double>(),v.at(3).get<double>(),v.at(4).get<double>(),v.at(5).get<std::vector<std::vector<std::array<double,2>>>>()});}validate(rs);return rs;}
}
struct RegionCache::Impl {
 std::mutex mutex;std::filesystem::path directory;std::size_t capacity;
 std::list<std::pair<std::string,std::vector<ColorRegion>>> memory;
 Impl(std::string d,std::size_t n):directory(std::move(d)),capacity(n){if(n<1||n>1024)throw std::invalid_argument("Invalid cache capacity");}
};
RegionCache::RegionCache(std::string directory,std::size_t entries):impl_(new Impl(std::move(directory),entries)){}
RegionCache::~RegionCache()=default;
void RegionCache::clear_memory(){std::lock_guard<std::mutex> lock(impl_->mutex);impl_->memory.clear();}
std::vector<ColorRegion> RegionCache::get(const AnalysisImage& image,double height,const std::string& algorithm,const std::function<std::vector<ColorRegion>()>& compute){
 if(image.width<1||image.height<1||image.width>2048||image.height>2048||image.rgba.size()!=std::size_t(image.width)*image.height*4||!std::isfinite(height)||height<=0||algorithm.empty())throw std::invalid_argument("Invalid analysis cache input");
 std::ostringstream meta;meta.imbue(std::locale::classic());meta<<algorithm<<'\n'<<image.width<<' '<<image.height<<' '<<std::setprecision(17)<<height<<'\n';
 std::string bytes=meta.str();bytes.append(reinterpret_cast<const char*>(image.rgba.data()),image.rgba.size());auto key=digest(bytes);
 // Native Vision already serializes device requests. This lock also coalesces
 // concurrent identical misses, including failure/retry; no stale success.
 std::lock_guard<std::mutex> lock(impl_->mutex);
 auto&memory=impl_->memory;auto found=std::find_if(memory.begin(),memory.end(),[&](const auto&v){return v.first==key;});
 if(found!=memory.end()){auto result=found->second;memory.splice(memory.begin(),memory,found);return result;}
 std::vector<ColorRegion> result;bool hit=false;
 const auto path=impl_->directory/(key+".fstrmask");
 if(!impl_->directory.empty())try{
  if(std::filesystem::is_regular_file(std::filesystem::symlink_status(path))&&std::filesystem::file_size(path)<=max_bytes){std::ifstream file(path,std::ios::binary);std::string data((std::istreambuf_iterator<char>(file)),{});auto j=Json::parse(data);auto payload=j.at("payload").get<std::string>();if(j.at("version")==1&&j.at("key")==key&&j.at("sha256")==digest(payload)){result=decode(payload);hit=true;}}
 }catch(const std::exception&){/* Invalid/unavailable cache is a miss, never analysis success. */}
 if(!hit){result=compute();auto payload=encode(result);
  if(!impl_->directory.empty())try{
   std::filesystem::create_directories(impl_->directory);
   auto data=Json{{"version",1},{"key",key},{"sha256",digest(payload)},{"payload",payload}}.dump();
   // Unique writer and atomic replacement under process-local lock. Existing
   // complete entries are immutable; concurrent processes need no shared state.
   auto pattern=path.string()+".tmp-XXXXXX";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
   int fd=mkstemp(name.data());if(fd<0)throw std::runtime_error("Cache temporary failed");
   struct Temporary {int fd;std::filesystem::path path;~Temporary(){if(fd>=0)::close(fd);std::error_code ec;std::filesystem::remove(path,ec);}} temporary{fd,name.data()};
   std::size_t done=0;while(done<data.size()){auto n=::write(fd,data.data()+done,data.size()-done);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("Cache write failed");done+=static_cast<std::size_t>(n);}
   if(::close(fd)!=0){temporary.fd=-1;throw std::runtime_error("Cache close failed");}temporary.fd=-1;
   std::filesystem::rename(temporary.path,path);
   std::vector<std::filesystem::directory_entry> entries;
   for(const auto&e:std::filesystem::directory_iterator(impl_->directory)){const auto name=e.path().filename().string();if(e.is_regular_file()&&!e.is_symlink()&&name.size()==73&&name.substr(64)==".fstrmask"&&std::all_of(name.begin(),name.begin()+64,[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');}))entries.push_back(e);}
   std::sort(entries.begin(),entries.end(),[](const auto&a,const auto&b){return a.last_write_time()<b.last_write_time();});
   for(std::size_t i=0;i+impl_->capacity<entries.size();++i)std::filesystem::remove(entries[i].path());
  }catch(const std::exception&){/* Cache persistence failure must not discard valid analysis. */}
 }
 memory.push_front({key,result});while(memory.size()>impl_->capacity)memory.pop_back();return result;
}
}
