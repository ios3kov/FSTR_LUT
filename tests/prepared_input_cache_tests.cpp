#include "prepared_input_cache.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
using namespace fstr::ae;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct State{int opaque=0,content=0;};
struct Value{int content=0;std::size_t bytes=8;std::size_t cache_bytes()const{return bytes;}};
bool same(State a,State b){return a.content==b.content;}
struct Gate{std::promise<void> p;std::shared_future<void> ready=p.get_future().share();bool open=false;void release(){if(!open){open=true;p.set_value();}}~Gate(){release();}};
}
int main(){try{
 auto cancel=[]{};int builds=0;auto build=[&]{return Value{++builds};};
 PreparedInputCache<State,Value> cache;bool hit=false;
 auto a=cache.get(State{1,10},"frame=1/prep=A",same,build,cancel,&hit);check(!hit&&builds==1,"Initial preparation missing");
 // Distinct opaque bytes may identify equal input; identical opaque bytes may
 // identify unequal input. Only the host comparator decides equality.
 auto b=cache.get(State{99,10},"frame=1/prep=A",same,build,cancel,&hit);check(hit&&a==b&&builds==1,"Compared state bytes rather than host input equality");
 auto c=cache.get(State{1,11},"frame=1/prep=A",same,build,cancel,&hit);check(!hit&&a!=c&&builds==2,"Reused changed input state");
 for(const auto* key:{"frame=2/prep=A","frame=1/prep=B","frame=1/type=16","frame=1/ROI=changed","frame=1/quality=changed"}){auto v=cache.get(State{1,10},key,same,build,cancel,&hit);check(!hit&&v!=a,"Context metadata ignored");}
 bool aborted=false;try{cache.get(State{1,10},"frame=1/prep=A",same,build,[]{throw std::runtime_error("cancel");});}catch(const std::runtime_error&){aborted=true;}check(aborted,"Resident hit ignored cancellation");
 int failed_builds=0;auto fail=[&]()->Value{++failed_builds;throw std::runtime_error("build failed");};for(int i=0;i<2;++i)try{cache.get(State{1,10},"failure",same,fail,cancel);}catch(const std::runtime_error&){}check(failed_builds==2,"Failure cached");
 // LRU count and byte bounds, with live borrowed values surviving eviction.
 {PreparedInputCache<State,Value> small(4096,2);int n=0;auto make=[&]{return Value{++n};};auto x=small.get(State{},"x",same,make,cancel);small.get(State{},"y",same,make,cancel);check(small.get(State{},"x",same,make,cancel)==x,"LRU refresh failed");small.get(State{},"z",same,make,cancel);small.get(State{},"y",same,make,cancel,&hit);check(!hit&&n==4&&x->content==1,"Count eviction or borrowed lifetime failed");}
 {PreparedInputCache<State,Value> small(512,10);int n=0;auto make=[&]{return Value{++n,300};};small.get(State{},"a",same,make,cancel);small.get(State{},"b",same,make,cancel);small.get(State{},"a",same,make,cancel,&hit);check(!hit&&n==3,"Byte bound ignored");auto huge=[&]{return Value{++n,4096};};small.get(State{},"large",same,huge,cancel);small.get(State{},"large",same,huge,cancel,&hit);check(!hit,"Oversize value retained");}
 // No mutex is held across build or equality. Clear rejects old publication.
 {PreparedInputCache<State,Value> concurrent;Gate gate;std::promise<void> entered;
 auto task=std::async(std::launch::async,[&]{return concurrent.get(State{},"a",same,[&]{entered.set_value();gate.ready.wait();return Value{7};},cancel);});
 auto status=entered.get_future().wait_for(2s);if(status!=std::future_status::ready)gate.release();check(status==std::future_status::ready,"Build did not enter");
 auto clearing=std::async(std::launch::async,[&]{concurrent.clear();});auto cleared=clearing.wait_for(2s);gate.release();clearing.get();auto old=task.get();auto fresh=concurrent.get(State{},"a",same,[]{return Value{8};},cancel,&hit);check(cleared==std::future_status::ready&&!hit&&fresh!=old&&old->content==7,"Clear blocked or old generation republished");
 auto compare=[&](State,State){concurrent.clear();return true;};check(concurrent.get(State{},"a",compare,build,cancel)==fresh,"Host equality callback held cache lock");}
 // Simultaneous resident hits are independent of an unrelated preparation.
 {PreparedInputCache<State,Value> concurrent;auto resident=concurrent.get(State{},"resident",same,[]{return Value{1};},cancel);Gate gate;std::promise<void> entered;auto miss=std::async(std::launch::async,[&]{return concurrent.get(State{},"miss",same,[&]{entered.set_value();gate.ready.wait();return Value{2};},cancel);});auto status=entered.get_future().wait_for(2s);if(status!=std::future_status::ready)gate.release();check(status==std::future_status::ready,"Miss did not enter");auto reader=std::async(std::launch::async,[&]{for(int i=0;i<1000;++i)check(concurrent.get(State{},"resident",same,build,cancel)==resident,"Concurrent hit changed value");});auto ready=reader.wait_for(2s);gate.release();miss.get();reader.get();check(ready==std::future_status::ready,"Hit waited for unrelated preparation");}
 std::cout<<"PASS state equality, explicit context, cancellation/failure, LRU/bytes, borrowed lifetime, clear and concurrency\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
