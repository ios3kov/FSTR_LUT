#include "fstr/deferred_resource.hpp"
#include "fstr/automatic_scene_cache.hpp"
#include <atomic>
#include <filesystem>
#include <future>
#include <iostream>
#include <thread>
#include <unistd.h>
using namespace fstr;using namespace std::chrono_literals;
namespace {
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
AnalysisImage image(unsigned char v){return {2,2,std::vector<std::uint8_t>(16,v)};}
std::shared_ptr<const SceneAnalysis> observation(const AnalysisImage&i){auto a=std::make_shared<SceneAnalysis>();a->width=i.width;a->height=i.height;a->scene.resize(4);return a;}
struct Model{int value=7;};
}
int main(){try{
 const auto root=std::filesystem::temp_directory_path()/("fstr-cache-first-"+std::to_string(getpid()));std::filesystem::create_directories(root);struct Guard{std::filesystem::path p;~Guard(){std::filesystem::remove_all(p);}} guard{root};const std::string policy(64,'a');
 {AutomaticSceneCache old(root.string(),policy,[](const auto&i,double,const auto&){return observation(i);});old.get(image(1),100);}
 std::atomic<int> starts{0};std::promise<void> entered,release;auto released=release.get_future().share();
 {DeferredResource<Model> untouched([&](const auto&){++starts;return std::make_shared<Model>();});bool cancelled=false;try{untouched.get([]{throw std::runtime_error("already cancelled");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled&&starts==0,"Already-cancelled caller started a model");}
 auto backend=std::make_shared<DeferredResource<Model>>([&](const auto& stop){++starts;entered.set_value();while(released.wait_for(1ms)!=std::future_status::ready)stop();return std::make_shared<Model>();});
 AutomaticSceneCache cache(root.string(),policy,[backend](const auto&i,double,const auto& cancel){check(backend->get(cancel)->value==7,"Wrong shared backend");return observation(i);},{},2);
 check(bool(cache.get(image(1),100))&&starts==0&&backend->state()==BackgroundResource<Model>::State::Idle,"Saved cache loaded a model");
 auto waiter=std::async(std::launch::async,[&]{int polls=0;try{cache.get(image(2),100,[&]{if(++polls>10)throw std::runtime_error("owned cancel");});return false;}catch(const std::runtime_error&e){return std::string(e.what())=="owned cancel";}});
 const auto entered_status=entered.get_future().wait_for(2s);if(entered_status!=std::future_status::ready)release.set_value();check(entered_status==std::future_status::ready,"Miss did not start backend");
 auto other=std::async(std::launch::async,[&]{return cache.get(image(3),100);});auto cancelled=waiter.wait_for(2s);release.set_value();check(cancelled==std::future_status::ready&&waiter.get(),"Cancelled waiter blocked");check(bool(other.get())&&starts==1,"Misses constructed multiple models");check(bool(cache.get(image(2),100))&&starts==1,"Cancelled miss poisoned cache/backend");
 std::atomic<int> failures{0};auto failed=std::make_shared<DeferredResource<Model>>([&](const auto&)->std::shared_ptr<Model>{++failures;throw std::runtime_error("model unavailable");});
 AutomaticSceneCache broken(root.string(),policy,[failed](const auto&i,double,const auto& cancel){failed->get(cancel);return observation(i);});
 for(int k=0;k<2;++k){bool threw=false;try{broken.get(image(4),100);}catch(const std::runtime_error&e){threw=std::string(e.what())=="model unavailable";}check(threw,"Model failure masked as success");}
 check(failures==1&&bool(broken.get(image(1),100)),"Failed model retried or invalidated valid persisted masks");
 std::atomic<bool> stopping{false};{
  auto shutdown=std::make_shared<DeferredResource<Model>>([&](const auto& stop)->std::shared_ptr<Model>{stopping=true;for(;;){stop();std::this_thread::sleep_for(1ms);}});
  bool cancelled_wait=false;try{shutdown->get([&]{if(stopping)throw std::runtime_error("cancel waiting");});}catch(const std::runtime_error&){cancelled_wait=true;}check(cancelled_wait,"Shutdown setup did not cancel waiting caller");
 } // Joins worker after shutdown cancellation; no detached factory survives.
 BackgroundResource<Model> ready;check(!ready.ready(),"Idle snapshot exposed value");ready.start([]{return std::make_shared<Model>();});auto value=ready.wait();check(ready.ready()==value,"Ready snapshot changed ownership");ready.stop();check(!ready.ready(),"Stopped snapshot exposed retired value");
 std::cout<<"PASS persisted masks without model start, singleton lazy miss, independent cancel, failure isolation, shutdown join and ready snapshots\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
