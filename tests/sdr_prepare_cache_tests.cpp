#include "fstr/sdr_prepare_cache.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <chrono>
#include <climits>
#include <iomanip>
#include <sstream>
#if defined(__APPLE__) || defined(__linux__)
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif
using namespace fstr;
void check(bool b,const char*s){if(!b)throw std::runtime_error(s);}
FrameView frame(const std::vector<float>&v){return {v.data(),v.size(),96,1,96*4,Transfer::Linear};}
int main(){const auto root=std::filesystem::temp_directory_path()/("fstr-prep-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{root};try{
 check(sdr_clip_anchor_times(72,1)==std::vector<std::int32_t>{0,35,71},"Wrong clip anchors");check(sdr_clip_anchor_times(18000,250)==std::vector<std::int32_t>{0,8750,17750},"Timing units confused with frame indices");check(sdr_clip_anchor_times(1,1)==std::vector<std::int32_t>{0},"Single-frame duplicate anchors");check(sdr_clip_anchor_times(INT32_MAX,INT32_MAX)==std::vector<std::int32_t>{0},"Anchor integer overflow");
 for(auto bad:std::vector<std::pair<int,int>>{{0,1},{1,0},{-1,2},{2,-1}}){bool rejected=false;try{sdr_clip_anchor_times(bad.first,bad.second);}catch(const std::invalid_argument&){rejected=true;}check(rejected,"Invalid timing accepted");}
 std::vector<float>a,b;for(int i=0;i<96;++i){const auto x=float(.04+.36*i/95);a.insert(a.end(),{x,x,x,1});b.insert(b.end(),{x+.001f,x+.001f,x+.001f,1});}
 SdrPreparationCache cache(root.string());auto first=cache.get({frame(a),frame(b)},"time0,35,71");check(first.use==SdrCacheUse::Computed&&first.shot.decision==SdrDecision::Prepared,"First preparation did not compute");
 auto again=cache.get({frame(b),frame(a)},"time0,35,71");check(again.key==first.key&&again.use==SdrCacheUse::Memory,"Reordered identical anchors missed RAM");
 SdrPreparationCache restarted(root.string());auto disk=restarted.get({frame(a),frame(b)},"time0,35,71");check(disk.use==SdrCacheUse::Disk&&disk.shot.settings.contrast==first.shot.settings.contrast&&disk.shot.settings.brightness_bias==first.shot.settings.brightness_bias,"Restart missed/changed recipe");
 auto changed=a;changed[0]+=.01f;check(cache.get({frame(changed),frame(b)},"time0,35,71").key!=first.key,"Changed upstream samples reused recipe");check(cache.get({frame(a),frame(b)},"time0,36,72").key!=first.key,"Changed timing reused recipe");
 auto padded=a;padded.insert(padded.end(),{99,98,97,96});auto f=frame(padded);f.row_stride=100*4;check(cache.get({f,frame(b)},"time0,35,71").use==SdrCacheUse::Memory,"Padding contaminated content identity");
 auto encoded=frame(a);encoded.transfer=Transfer::SRGB;check(cache.get({encoded},"transfer").key!=cache.get({frame(a)},"transfer").key,"Transfer change reused recipe");
 bool oversized=false;try{auto invalid=frame(a);invalid.width=97;cache.get({invalid},"oversized");}catch(const std::invalid_argument&){oversized=true;}check(oversized,"Oversized snapshot accepted");
 const auto path=root/(first.key+".fstrprep");{std::ofstream o(path);o<<"corrupt";}check(cache.get({frame(a),frame(b)},"time0,35,71").use==SdrCacheUse::Memory,"RAM read corrupted disk");cache.clear_memory();check(cache.get({frame(a),frame(b)},"time0,35,71").use==SdrCacheUse::Computed,"Corrupt recipe accepted");
 {std::ifstream input(path);std::string text((std::istreambuf_iterator<char>(input)),{});const auto at=text.find("\"version\":1");check(at!=std::string::npos,"Schema fixture unavailable");text[at+10]='2';std::ofstream out(path);out<<text;}
 cache.clear_memory();check(cache.get({frame(a),frame(b)},"time0,35,71").use==SdrCacheUse::Computed,"Unknown disk schema accepted");
 std::vector<std::future<CachedSdrPreparation>> jobs;for(int i=0;i<8;++i)jobs.push_back(std::async(std::launch::async,[&]{return cache.get({frame(a)},"coalesce");}));int computes=0;for(auto& j:jobs)computes+=j.get().use==SdrCacheUse::Computed;check(computes==1,"Concurrent preparation repeated compute");
 bool cancelled=false;try{cache.get({frame(a)},"coalesce",[]{throw std::runtime_error("cancelled");});}catch(const std::runtime_error&){cancelled=true;}check(cancelled,"RAM hit ignored cancellation");
 SdrPreparationCache ram;ram.get({frame(a)},"oldest");for(int i=0;i<64;++i)ram.get({frame(a)},"ram-"+std::to_string(i));check(ram.get({frame(a)},"ram-0").use==SdrCacheUse::Memory&&ram.get({frame(a)},"oldest").use==SdrCacheUse::Computed,"RAM retention exceeded or lost 64-entry boundary");
 std::filesystem::create_directories(root/"foreign-dir");{std::ofstream o(root/"foreign.txt");o<<"keep";}const auto orphan=root/(".pending-v1-"+std::string(64,'0')+"-1-2");{std::ofstream o(orphan);o<<"partial";}cache.get({frame(b)},"cleanup");
#if defined(__APPLE__) || defined(__linux__)
 check(!std::filesystem::exists(orphan),"Abandoned owned temporary survived");
#endif
 check(std::filesystem::exists(root/"foreign.txt")&&std::filesystem::exists(root/"foreign-dir"),"Foreign paths deleted");
#if defined(__APPLE__) || defined(__linux__)
 // Recovery must neither remove another publisher's temporary nor claim persistence when locked.
 const int fd=::open((root/".write-v1.lock").c_str(),O_RDWR);check(fd>=0&&::flock(fd,LOCK_EX|LOCK_NB)==0,"Test lock unavailable");
 {std::ofstream o(orphan);o<<"active publisher";}
 SdrPreparationCache busy(root.string());auto transient=busy.get({frame(a)},"busy-lock");check(transient.use==SdrCacheUse::Computed&&std::filesystem::exists(orphan)&&!std::filesystem::exists(root/(transient.key+".fstrprep")),"Busy lock damaged publisher or reported persistence");
 ::close(fd);const auto bad=root/(".pending-v1-"+std::string(64,'0')+"-01-2");{std::ofstream o(bad);o<<"foreign";}
 const auto link=root/(".pending-v1-"+std::string(64,'0')+"-3-4");std::filesystem::create_symlink(root/"foreign.txt",link);
 SdrPreparationCache recovered(root.string());check(!std::filesystem::exists(orphan)&&std::filesystem::exists(bad)&&std::filesystem::is_symlink(link),"Recovery crossed ownership boundary");
 auto named=[](int n){std::ostringstream s;s<<std::hex<<std::setw(64)<<std::setfill('0')<<n;return s.str()+".fstrprep";};
 const auto count_root=root/"count-budget";std::filesystem::create_directory(count_root);
 for(int i=1;i<=2049;++i){std::ofstream o(count_root/named(i));o<<"fixture";o.close();std::filesystem::last_write_time(count_root/named(i),std::filesystem::file_time_type::clock::now()-std::chrono::hours(2)+std::chrono::seconds(i));}
 SdrPreparationCache bounded(count_root.string());auto published=bounded.get({frame(a)},"count-budget");std::size_t count=0;for(const auto& e:std::filesystem::directory_iterator(count_root))count+=e.path().extension()==".fstrprep";
 check(count==2048&&!std::filesystem::exists(count_root/named(1))&&!std::filesystem::exists(count_root/named(2))&&std::filesystem::exists(count_root/(published.key+".fstrprep")),"Count retention failed oldest-first boundary");
 const auto byte_root=root/"byte-budget";std::filesystem::create_directory(byte_root);
 for(int i=1;i<=2;++i){std::ofstream o(byte_root/named(i));o.seekp(5*1024*1024-1);o.put('x');o.close();std::filesystem::last_write_time(byte_root/named(i),std::filesystem::file_time_type::clock::now()-std::chrono::hours(2)+std::chrono::seconds(i));}
 SdrPreparationCache bytes(byte_root.string());auto fresh=bytes.get({frame(a)},"byte-budget");std::uintmax_t total=0;for(const auto&e:std::filesystem::directory_iterator(byte_root))if(e.path().extension()==".fstrprep")total+=e.file_size();
 check(total<=8*1024*1024&&!std::filesystem::exists(byte_root/named(1))&&std::filesystem::exists(byte_root/(fresh.key+".fstrprep")),"Byte retention failed oldest-first boundary");
#endif
 const auto blocked=root/"blocked";{std::ofstream o(blocked);o<<"not directory";}SdrPreparationCache unavailable(blocked.string());check(unavailable.get({frame(a)},"fallback").shot.decision==SdrDecision::Prepared,"Persistence failure discarded analysis");
 std::cout<<"PASS preparation cache: fixed times, canonical content, RAM/disk/restart, upstream/timing invalidation, corruption, coalescing, cancellation, locked publisher, owned recovery, count/byte retention, persistence fallback\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
