#include "fstr/automatic_scene_cache.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <future>
#include <atomic>
#include <chrono>
#include <stdexcept>
#if defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#endif
using namespace fstr;
void check(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}
#if defined(__APPLE__) || defined(__linux__)
void recovery_contracts(const std::filesystem::path&root,const std::string&policy,const SceneCompute&compute,const AnalysisImage&im){
 const auto dir=root/"process-recovery",orphan=dir/".building-v1-12-34";
 const auto lock_path=(dir/".fstr-write-v1.lock").string(),orphan_path=orphan.string(),partial_path=(orphan/"partial.weights").string();
 std::filesystem::create_directories(dir);
 int ready[2],release[2];check(::pipe(ready)==0,"Ready pipe failed");check(::pipe(release)==0,"Release pipe failed");
 auto child=::fork();check(child>=0,"Writer fork failed");
 if(child==0){
  ::close(ready[0]);::close(release[1]);::alarm(15);
  int fd=::open(lock_path.c_str(),O_CREAT|O_RDWR,0600);
  if(fd<0||::flock(fd,LOCK_EX|LOCK_NB)!=0||::mkdir(orphan_path.c_str(),0700)!=0)::_exit(2);
  int part=::open(partial_path.c_str(),O_CREAT|O_WRONLY,0600);if(part<0)::_exit(3);::close(part);
  char value='r';if(::write(ready[1],&value,1)!=1||::read(release[0],&value,1)!=1)::_exit(4);
  // Abrupt process exit: no C++ cleanup and no explicit unlock/close of fd.
  ::_exit(23);
 }
 ::close(ready[1]);::close(release[0]);
 struct ChildGuard{pid_t pid;int ready,release;~ChildGuard(){if(pid>0){::kill(pid,SIGKILL);::waitpid(pid,nullptr,0);}if(ready>=0)::close(ready);if(release>=0)::close(release);}} guard{child,ready[0],release[1]};
 char value=0;check(::read(ready[0],&value,1)==1&&value=='r',"Writer did not hold root lock");
 AutomaticSceneCache busy(dir.string(),policy,compute);
 check(std::filesystem::exists(orphan/"partial.weights"),"Recovery deleted live writer's files");
 check(busy.get(im,100)->people.size()==1,"Busy disk lock discarded valid analysis");
 check(!std::filesystem::exists(dir/(scene_cache_key(im,100,policy)+".entry"))&&std::filesystem::exists(orphan),"Publication bypassed live writer lock");
 check(::write(release[1],&value,1)==1,"Writer release failed");int status=0;check(::waitpid(child,&status,0)==child,"Writer reap failed");guard.pid=-1;
 check(WIFEXITED(status)&&WEXITSTATUS(status)==23,"Writer did not simulate interrupted cleanup");
 // Preserve legacy/unrecognized names and never follow directory symlinks.
 for(const auto&name:{".building-12-34",".building-v1-01-2",".building-v1-4294967296-2",".building-v1-1-2-extra","foreign","valid.entry"})std::filesystem::create_directories(dir/name);
 const auto outside=root/"outside-recovery";std::filesystem::create_directories(outside);{std::ofstream f(outside/"keep");f<<"keep";}
 std::filesystem::create_directory_symlink(outside,dir/".building-v1-56-78");
 std::filesystem::create_directory_symlink(outside,orphan/"external");
 AutomaticSceneCache recovered(dir.string(),policy,compute);
 check(!std::filesystem::exists(orphan)&&std::filesystem::exists(outside/"keep"),"Interrupted write not removed or external link followed");
 check(std::filesystem::is_symlink(std::filesystem::symlink_status(dir/".building-v1-56-78")),"Managed-name symlink was removed");
 for(const auto&name:{".building-12-34",".building-v1-01-2",".building-v1-4294967296-2",".building-v1-1-2-extra","foreign","valid.entry"})check(std::filesystem::exists(dir/name),"Foreign or legacy path was removed");
 auto saved=recovered.get(im,100);AutomaticSceneCache restarted(dir.string(),policy,compute);
 check(restarted.get(im,100)->people[0].mask==saved->people[0].mask&&std::filesystem::exists(dir/(scene_cache_key(im,100,policy)+".entry")),"Recovery removed complete analysis");
 // Recovery also runs on the next publication after construction.
 std::filesystem::create_directories(orphan);auto changed=im;++changed.rgba[0];recovered.get(changed,100);
 check(!std::filesystem::exists(orphan),"Publication did not recover abandoned write");
 const auto unsafe=root/"unsafe-lock";std::filesystem::create_directories(unsafe/".building-v1-1-2");std::filesystem::create_symlink(outside/"keep",unsafe/".fstr-write-v1.lock");
 AutomaticSceneCache blocked(unsafe.string(),policy,compute);check(blocked.get(im,100)->people.size()==1&&std::filesystem::exists(unsafe/".building-v1-1-2"),"Symlink lock disabled analysis or allowed deletion");
 std::cout<<"PASS recovery: live process exclusion, interrupted writer, preserved entries/legacy/foreign/symlinks, busy persistence fallback\n";
}
#endif
int main(){const auto root=std::filesystem::temp_directory_path()/("fstr-auto-cache-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));try{
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{root};const std::string policy(64,'a');AnalysisImage im{2,1,{10,20,30,255,40,50,60,255}};std::atomic<int> calls{0};auto compute=[&](const AnalysisImage&i,double,const std::function<void()>&abort){if(abort)abort();++calls;auto v=std::make_shared<SceneAnalysis>();v->width=i.width;v->height=i.height;v->scene.resize(2);for(auto&w:v->scene)w[0]=.7f;PersonAnalysis p;p.mask={.5f,.6f};p.toning_support={.8f,.9f};p.toning_strength=.3;p.box={.1,.2,.8,.9};p.descriptor={1.1,.9,.8};p.detection_confidence=.95;p.samples={{{.1,.2,.3},.75}};v->people.push_back(p);return v;};
#if defined(__APPLE__) || defined(__linux__)
 const auto recovery=root/"recovery",orphan=recovery/".building-v1-123-456";
 std::filesystem::create_directories(orphan);{std::ofstream f(orphan/"partial.weights");f<<"unfinished";}
 AutomaticSceneCache recovered(recovery.string(),policy,compute);
 check(!std::filesystem::exists(orphan),"Abandoned managed cache write survived startup");
 recovery_contracts(root,policy,compute,im);calls=0;
#endif
 AutomaticSceneCache cache(root.string(),policy,compute);auto a=cache.get(im,100);check(calls==1&&a->people.size()==1,"Unseen image did not compute");auto b=cache.get(im,100);check(a==b&&calls==1,"Repeat did not reuse memory");cache.clear_memory();auto disk=cache.get(im,100);check(calls==1&&disk->people[0].box==a->people[0].box&&disk->people[0].descriptor==a->people[0].descriptor&&disk->people[0].detection_confidence==.95&&disk->people[0].toning_strength==.3,"Disk result differs or recomputed");
 AutomaticSceneCache restarted(root.string(),policy,compute);check(restarted.get(im,100)->people[0].toning_support==a->people[0].toning_support&&calls==1,"Restart missed cache");auto changed=im;changed.rgba[0]++;restarted.get(changed,100);check(calls==2,"Changed pixels reused analysis");restarted.get(im,101);check(calls==3,"Changed source scale reused analysis");AutomaticSceneCache revision(root.string(),std::string(64,'b'),compute);revision.get(im,100);check(calls==4,"Changed policy reused analysis");
 auto same=im;same.rgba[0]+=2;std::vector<std::future<std::shared_ptr<const SceneAnalysis>>> jobs;for(int j=0;j<8;++j)jobs.push_back(std::async(std::launch::async,[&]{return cache.get(same,100);}));for(auto&j:jobs)check(j.get()->people.size()==1,"Concurrent result missing");check(calls==5,"Concurrent identical misses repeated inference");
 auto cancel_im=im;cancel_im.rgba[0]+=3;bool cancelled=false;try{cache.get(cancel_im,100,[]{throw std::runtime_error("cancelled");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled&&calls==5,"Cancelled request invoked model or returned success");
 auto key=scene_cache_key(im,100,policy);{std::ofstream f(root/(key+".entry")/(key+".weights"),std::ios::binary);f<<"bad";}cache.clear_memory();auto repaired=cache.get(im,100);check(calls==6&&repaired->people[0].mask==a->people[0].mask,"Corrupt entry was accepted or not recomputed");
 auto bad=[&](const AnalysisImage&i,double,const std::function<void()>&){auto v=compute(i,1,{});v->people[0].mask[0]=1;return v;};AutomaticSceneCache invalid((root/"invalid").string(),policy,bad);bool rejected=false;try{invalid.get(im,100);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid ownership was cached");
 auto before=calls.load();cache.get(im,100,{},std::string(64,'c'));cache.get(im,100,{},std::string(64,'c'));cache.get(im,100,{},std::string(64,'d'));check(calls==before+2,"Exact upstream identity did not invalidate or reuse");
 // A short 72-frame clip must survive disk-only replay after a process restart.
 // Tiny deterministic analyses isolate retention from model speed/variability.
 const auto clip_root=root/"clip-72";auto baseline=calls.load();
 {AutomaticSceneCache first(clip_root.string(),policy,compute);for(int k=0;k<72;++k){auto frame=im;frame.rgba[1]=std::uint8_t(k);first.get(frame,100);}}
 check(calls==baseline+72,"Clip fixture did not analyze every distinct frame");
 {AutomaticSceneCache replay(clip_root.string(),policy,compute);for(int k=0;k<72;++k){auto frame=im;frame.rgba[1]=std::uint8_t(k);check(replay.get(frame,100)->people[0].mask==a->people[0].mask,"Clip replay changed masks");}}
 check(calls==baseline+72,"72-frame disk replay repeated analysis below byte budget");
 std::filesystem::create_directories(root/"keep_me");{std::ofstream f(root/"foreign.txt");f<<"keep";}for(int k=0;k<67;++k){auto unique=im;unique.rgba[1]=static_cast<std::uint8_t>(k+80);cache.get(unique,100);}std::size_t entries=0;for(const auto&e:std::filesystem::directory_iterator(root))if(e.path().extension()==".entry")++entries;check(entries>64&&entries<=4096&&std::filesystem::exists(root/"foreign.txt")&&std::filesystem::exists(root/"keep_me"),"Disk entry bound or foreign-file preservation failed");
 // Oversized oldest managed payload must still be evicted by the 512 MiB budget.
 // Sparse files test logical accounting without allocating hundreds of MiB.
 const auto byte_root=root/"byte-budget";const std::string old_key(64,'0');
 const auto old_dir=byte_root/(old_key+".entry");std::filesystem::create_directories(old_dir);
 {std::ofstream f(old_dir/"old.weights",std::ios::binary);f<<'x';}
 std::filesystem::resize_file(old_dir/"old.weights",513ULL*1024*1024);
 std::filesystem::last_write_time(old_dir,std::filesystem::file_time_type::clock::now()-std::chrono::hours(1));
 {AutomaticSceneCache bounded(byte_root.string(),policy,compute);bounded.get(im,100);}
 check(!std::filesystem::exists(old_dir)&&std::filesystem::exists(byte_root/(key+".entry")),"Disk byte budget did not evict oldest payload or removed current entry");
 // Seed tiny managed entries to exercise the separate inode/count ceiling.
 const auto count_root=root/"count-budget";std::filesystem::create_directories(count_root);
 for(unsigned n=0;n<4096;++n){auto k=std::string(64,'0');for(unsigned j=0;j<4;++j)k[63-j]="0123456789abcdef"[(n>>(4*j))&15];std::filesystem::create_directory(count_root/(k+".entry"));}
 const auto oldest=count_root/(std::string(64,'0')+".entry");
 std::filesystem::last_write_time(oldest,std::filesystem::file_time_type::clock::now()-std::chrono::hours(1));
 {AutomaticSceneCache bounded(count_root.string(),policy,compute);bounded.get(im,100);}
 std::size_t count=0;for(const auto&e:std::filesystem::directory_iterator(count_root))if(e.path().extension()==".entry")++count;
 check(count==4096&&!std::filesystem::exists(oldest)&&std::filesystem::exists(count_root/(key+".entry")),"Tiny-entry ceiling or oldest-write eviction failed");
 AutomaticSceneCache unavailable((root/"blocked").string(),policy,compute);{std::ofstream f(root/"blocked");f<<"not a directory";}check(unavailable.get(im,100)->people.size()==1,"Persistence failure discarded valid analysis");
 std::cout<<"PASS automatic cache: miss, RAM, disk, restart, pixels, scale, policy, coalescing, cancellation, corruption, validation, unavailable persistence\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
