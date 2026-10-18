#include "fstr/background_resource.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace std::chrono_literals;
namespace {
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
struct Item{std::atomic<int>* destroyed;int value;~Item(){++*destroyed;}};
using Loader=fstr::BackgroundResource<Item>;
}
int main(){try{
 std::atomic<int> destroyed{0},built{0};Loader loader;
 std::promise<void> entered,release;auto gate=release.get_future().share();
 auto owner=std::this_thread::get_id();
 loader.start([&]{check(std::this_thread::get_id()!=owner,"Factory ran on caller");++built;entered.set_value();gate.wait();return std::shared_ptr<Item>(new Item{&destroyed,73});});
 entered.get_future().wait();check(loader.state()==Loader::State::Loading,"Start waited for blocked factory");
 loader.start([&]{++built;return std::shared_ptr<Item>{};});check(built==1,"Duplicate initialization");
 int polls=0;bool cancelled=false;try{loader.wait([&]{if(++polls==3)throw 75;});}catch(int n){cancelled=n==75;}check(cancelled&&loader.state()==Loader::State::Loading,"Cancellation killed loading or failed");
 std::vector<std::future<std::shared_ptr<Item>>> readers;for(int i=0;i<8;++i)readers.push_back(std::async(std::launch::async,[&]{return loader.wait();}));
 release.set_value();auto first=loader.wait();check(first->value==73,"Wrong publication");for(auto&f:readers)check(f.get()==first,"Readers got different resources");
 loader.stop();check(loader.state()==Loader::State::Stopped&&destroyed==0,"Stop destroyed retained request");first.reset();check(destroyed==1,"Request leaked resource");
 loader.start([]()->std::shared_ptr<Item>{throw std::runtime_error("known failure");});bool failed=false;try{loader.wait();}catch(const std::runtime_error&e){failed=std::string(e.what())=="known failure";}check(failed&&loader.state()==Loader::State::Failed,"Failure masked as readiness");loader.stop();
 std::promise<void> entered2,release2;auto gate2=release2.get_future().share();loader.start([&]{entered2.set_value();gate2.wait();return std::shared_ptr<Item>(new Item{&destroyed,9});});entered2.get_future().wait();
 auto stop=std::async(std::launch::async,[&]{loader.stop();});
 const auto deadline=std::chrono::steady_clock::now()+2s;while(loader.state()!=Loader::State::Stopped&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();check(loader.state()==Loader::State::Stopped,"Stop did not wake state");
 bool stopped=false;try{loader.wait();}catch(const std::runtime_error&){stopped=true;}check(stopped,"Stopped waiter succeeded");check(stop.wait_for(20ms)==std::future_status::timeout,"Stop abandoned factory");release2.set_value();stop.get();check(destroyed==2,"Late result survived join");
 loader.start([]{return std::shared_ptr<Item>{};});bool empty=false;try{loader.wait();}catch(const std::runtime_error&){empty=true;}check(empty,"Null resource published");loader.stop();
 for(int round=0;round<40;++round){loader.start([&]{return std::shared_ptr<Item>(new Item{&destroyed,round});});auto v=loader.wait();check(v->value==round,"Restart stale state");loader.stop();}check(destroyed==42,"Repeated lifecycle leak");
 {Loader scoped;std::promise<void> e;scoped.start([&]{e.set_value();return std::shared_ptr<Item>(new Item{&destroyed,1});});e.get_future().wait();}check(destroyed==43,"Destructor did not join");
 std::cout<<"PASS background factory, one publication,8readers, waiter cancellation, failure/null, stop wake/join, retained lifetime,40restarts and destruction\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
