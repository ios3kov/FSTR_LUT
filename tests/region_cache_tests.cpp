#include "fstr/region_cache.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>
void check(bool b,const char*message){if(!b)throw std::runtime_error(message);}
int main(){std::filesystem::path dir=std::filesystem::temp_directory_path()/("fstr-cache-test-"+std::to_string(getpid()));try{
 std::filesystem::create_directory(dir);fstr::AnalysisImage image{8,8,std::vector<std::uint8_t>(256,128)};std::atomic<int> calls{0};auto compute=[&]{++calls;return std::vector<fstr::ColorRegion>{{.1,.1,.8,.8,.5,{}}};};
 fstr::RegionCache cache(dir.string(),2);auto first=cache.get(image,1080,"model-v1",compute);check(calls==1,"Initial analysis missing");
 // Smart Off never calls get; switching back or changing look uses the same key.
 auto second=cache.get(image,1080,"model-v1",compute);check(calls==1&&first[0].x0==second[0].x0,"Smart re-enable repeated analysis");
 cache.clear_memory();cache.get(image,1080,"model-v1",compute);check(calls==1,"Disk cache miss after memory clear");
 {fstr::RegionCache reopened(dir.string(),2);reopened.get(image,1080,"model-v1",compute);}check(calls==1,"Cache lost across instances");
 image.rgba[0]++;cache.get(image,1080,"model-v1",compute);check(calls==2,"Changed pixels not invalidated");cache.get(image,2160,"model-v1",compute);check(calls==3,"Scale not invalidated");cache.get(image,2160,"model-v2",compute);check(calls==4,"Model version not invalidated");
 std::size_t count=0;for(auto&e:std::filesystem::directory_iterator(dir))if(e.path().extension()==".fstrmask")++count;check(count<=2,"Disk cache limit exceeded");
 for(auto&e:std::filesystem::directory_iterator(dir)){std::ofstream file(e.path());file<<"corrupt";}
 cache.clear_memory();cache.get(image,2160,"model-v2",compute);check(calls==5,"Corrupt cache accepted");
 cache.clear_memory();std::thread a([&]{cache.get(image,1080,"parallel-v1",compute);}),b([&]{cache.get(image,1080,"parallel-v1",compute);});a.join();b.join();check(calls==6,"Concurrent duplicate analysis");
 bool failed=false;try{cache.get(image,1080,"failure",[]{throw std::runtime_error("detector failed");return std::vector<fstr::ColorRegion>{};});}catch(...){failed=true;}check(failed,"Failure masked");cache.get(image,1080,"failure",compute);check(calls==7,"Failure stored as success");
 {fstr::RegionCache empty((dir/"empty").string(),2);int n=0;auto none=[&]{++n;return std::vector<fstr::ColorRegion>{};};empty.get(image,1080,"none",none);empty.clear_memory();empty.get(image,1080,"none",none);check(n==1,"Valid empty result not cached");}
 {auto blocked=dir/"blocked";{std::ofstream f(blocked);f<<"not a directory";}fstr::RegionCache unavailable(blocked.string(),2);unavailable.get(image,1080,"blocked",compute);check(calls==8,"Unavailable disk prevented analysis");}
 for(const auto&e:std::filesystem::directory_iterator(dir))check(e.path().string().find(".tmp-")==std::string::npos,"Temporary leaked");
 std::filesystem::remove_all(dir);std::cout<<"PASS: native analysis memory/disk/content/version/concurrency/failure contracts\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';std::filesystem::remove_all(dir);return 1;}}
