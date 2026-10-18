#include "fstr/automatic_scene_cache.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <future>
#include <atomic>
#include <chrono>
#include <stdexcept>
using namespace fstr;
void check(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}
int main(){const auto root=std::filesystem::temp_directory_path()/("fstr-auto-cache-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));try{
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{root};const std::string policy(64,'a');AnalysisImage im{2,1,{10,20,30,255,40,50,60,255}};std::atomic<int> calls{0};auto compute=[&](const AnalysisImage&i,double,const std::function<void()>&abort){if(abort)abort();++calls;auto v=std::make_shared<SceneAnalysis>();v->width=i.width;v->height=i.height;v->scene.resize(2);for(auto&w:v->scene)w[0]=.7f;PersonAnalysis p;p.mask={.5f,.6f};p.toning_support={.8f,.9f};p.toning_strength=.3;p.box={.1,.2,.8,.9};p.descriptor={1.1,.9,.8};p.detection_confidence=.95;p.samples={{{.1,.2,.3},.75}};v->people.push_back(p);return v;};
 AutomaticSceneCache cache(root.string(),policy,compute);auto a=cache.get(im,100);check(calls==1&&a->people.size()==1,"Unseen image did not compute");auto b=cache.get(im,100);check(a==b&&calls==1,"Repeat did not reuse memory");cache.clear_memory();auto disk=cache.get(im,100);check(calls==1&&disk->people[0].box==a->people[0].box&&disk->people[0].descriptor==a->people[0].descriptor&&disk->people[0].detection_confidence==.95&&disk->people[0].toning_strength==.3,"Disk result differs or recomputed");
 AutomaticSceneCache restarted(root.string(),policy,compute);check(restarted.get(im,100)->people[0].toning_support==a->people[0].toning_support&&calls==1,"Restart missed cache");auto changed=im;changed.rgba[0]++;restarted.get(changed,100);check(calls==2,"Changed pixels reused analysis");restarted.get(im,101);check(calls==3,"Changed source scale reused analysis");AutomaticSceneCache revision(root.string(),std::string(64,'b'),compute);revision.get(im,100);check(calls==4,"Changed policy reused analysis");
 auto same=im;same.rgba[0]+=2;std::vector<std::future<std::shared_ptr<const SceneAnalysis>>> jobs;for(int j=0;j<8;++j)jobs.push_back(std::async(std::launch::async,[&]{return cache.get(same,100);}));for(auto&j:jobs)check(j.get()->people.size()==1,"Concurrent result missing");check(calls==5,"Concurrent identical misses repeated inference");
 auto cancel_im=im;cancel_im.rgba[0]+=3;bool cancelled=false;try{cache.get(cancel_im,100,[]{throw std::runtime_error("cancelled");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled&&calls==5,"Cancelled request invoked model or returned success");
 auto key=scene_cache_key(im,100,policy);{std::ofstream f(root/(key+".entry")/(key+".weights"),std::ios::binary);f<<"bad";}cache.clear_memory();auto repaired=cache.get(im,100);check(calls==6&&repaired->people[0].mask==a->people[0].mask,"Corrupt entry was accepted or not recomputed");
 auto bad=[&](const AnalysisImage&i,double,const std::function<void()>&){auto v=compute(i,1,{});v->people[0].mask[0]=1;return v;};AutomaticSceneCache invalid((root/"invalid").string(),policy,bad);bool rejected=false;try{invalid.get(im,100);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid ownership was cached");
 auto before=calls.load();cache.get(im,100,{},std::string(64,'c'));cache.get(im,100,{},std::string(64,'c'));cache.get(im,100,{},std::string(64,'d'));check(calls==before+2,"Exact upstream identity did not invalidate or reuse");
 std::filesystem::create_directories(root/"keep_me");{std::ofstream f(root/"foreign.txt");f<<"keep";}for(int k=0;k<67;++k){auto unique=im;unique.rgba[1]=static_cast<std::uint8_t>(k+80);cache.get(unique,100);}std::size_t entries=0;for(const auto&e:std::filesystem::directory_iterator(root))if(e.path().extension()==".entry")++entries;check(entries<=64&&std::filesystem::exists(root/"foreign.txt")&&std::filesystem::exists(root/"keep_me"),"Disk entry bound or foreign-file preservation failed");
 AutomaticSceneCache unavailable((root/"blocked").string(),policy,compute);{std::ofstream f(root/"blocked");f<<"not a directory";}check(unavailable.get(im,100)->people.size()==1,"Persistence failure discarded valid analysis");
 std::cout<<"PASS automatic cache: miss, RAM, disk, restart, pixels, scale, policy, coalescing, cancellation, corruption, validation, unavailable persistence\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
