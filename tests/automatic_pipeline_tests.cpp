#include "fstr/automatic_scene_cache.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace fstr;using namespace std::chrono_literals;
namespace {
AnalysisImage image(int n){return {2,1,{std::uint8_t(n),20,30,255,40,50,60,255}};}
std::shared_ptr<const SceneAnalysis> result(const AnalysisImage&i){auto a=std::make_shared<SceneAnalysis>();a->width=i.width;a->height=i.height;a->scene.resize(2);return a;}
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
}
int main(){auto root=std::filesystem::temp_directory_path()/("fstr-pipeline-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};try{const std::string policy(64,'f');
for(unsigned limit:{0u,3u}){bool threw=false;try{AutomaticSceneCache c((root/"invalid").string(),policy,[](const auto&i,double,const auto&){return result(i);},{},limit);}catch(const std::invalid_argument&){threw=true;}check(threw,"Invalid compute limit accepted");}
{
 std::promise<void> release;auto wait=release.get_future().share();std::atomic<int> active{0},peak{0},entered{0},calls{0};
 AutomaticSceneCache cache((root/"bounded").string(),policy,[&](const auto&i,double,const auto&){++calls;if(i.rgba[0]!=1){int count=++active;int previous=peak.load();while(count>previous&&!peak.compare_exchange_weak(previous,count)){}++entered;wait.wait();--active;}return result(i);},{},2);
 cache.get(image(1),100);cache.clear_memory();
 auto a=std::async(std::launch::async,[&]{return cache.get(image(2),100);});auto b=std::async(std::launch::async,[&]{return cache.get(image(3),100);});
 const auto deadline=std::chrono::steady_clock::now()+2s;while(entered<2&&std::chrono::steady_clock::now()<deadline)std::this_thread::sleep_for(1ms);
 auto third=std::async(std::launch::async,[&]{return cache.get(image(4),100);});auto disk=std::async(std::launch::async,[&]{return cache.get(image(1),100);});
 auto disk_status=disk.wait_for(2s),third_status=third.wait_for(20ms);bool two=entered==2;release.set_value();(void)a.get();(void)b.get();(void)third.get();auto hit=disk.get();
 check(two&&peak==2&&third_status==std::future_status::timeout,"Compute limit not exactly two");check(disk_status==std::future_status::ready&&hit&&calls==4,"Disk hit blocked behind full compute lanes or reanalyzed");
 cache.clear_memory();int before=calls;cache.get(image(2),100);cache.get(image(3),100);cache.get(image(4),100);check(calls==before,"Concurrent writes dropped persistent observations");
}
{
 std::promise<void> release,entered;auto wait=release.get_future().share();std::atomic<int> calls{0};AutomaticSceneCache cache((root/"join").string(),policy,[&](const auto&i,double,const auto&){if(++calls==1)entered.set_value();wait.wait();return result(i);},{},2);
 auto owner=std::async(std::launch::async,[&]{return cache.get(image(8),100);});auto status=entered.get_future().wait_for(2s);if(status!=std::future_status::ready)release.set_value();check(status==std::future_status::ready,"Owner not started");
 std::vector<std::future<std::shared_ptr<const SceneAnalysis>>> joined;for(int n=0;n<8;++n)joined.push_back(std::async(std::launch::async,[&]{return cache.get(image(8),100);}));
 auto cancel=std::async(std::launch::async,[&]{int polls=0;try{cache.get(image(8),100,[&]{if(++polls==3)throw std::runtime_error("cancelled waiter");});return false;}catch(const std::runtime_error&e){return std::string(e.what())=="cancelled waiter";}});auto cancelled=cancel.wait_for(2s);release.set_value();auto expected=owner.get();bool same=true;for(auto&f:joined)same&=f.get()==expected;
 check(cancelled==std::future_status::ready&&cancel.get()&&same&&calls==1,"Single flight or independent waiter cancellation failed");
}
{
 std::promise<void> release_old,release_new,entered_old,entered_new;auto old_ready=release_old.get_future().share(),new_ready=release_new.get_future().share();std::atomic<int> calls{0};
 AutomaticSceneCache cache((root/"generation-two").string(),policy,[&](const auto&i,double,const auto&){if(++calls==1){entered_old.set_value();old_ready.wait();}else{entered_new.set_value();new_ready.wait();}return result(i);},{},2);
 auto owner=std::async(std::launch::async,[&]{return cache.get(image(11),100);});auto first=entered_old.get_future().wait_for(2s);if(first!=std::future_status::ready){release_old.set_value();release_new.set_value();}check(first==std::future_status::ready,"Old generation did not start");
 cache.clear_memory();auto fresh=std::async(std::launch::async,[&]{return cache.get(image(11),100);});auto second=entered_new.get_future().wait_for(2s);release_new.set_value();if(second!=std::future_status::ready)release_old.set_value();auto newer=fresh.get();if(second==std::future_status::ready)release_old.set_value();auto older=owner.get();
 check(second==std::future_status::ready&&calls==2&&newer!=older&&cache.get(image(11),100)==newer,"Clear joined an old flight or allowed old RAM publication");
}
std::cout<<"PASS two-compute bound, independent disk hit, serialized persistence, single-flight join and waiter cancellation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
