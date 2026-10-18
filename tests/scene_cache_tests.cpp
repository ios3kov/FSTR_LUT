#include "scene_cache_fixture.hpp"
#include <future>
#include <iostream>
#include <unistd.h>
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
int main(){char pattern[]="/tmp/fstr-scene-cache-XXXXXX";auto name=mkdtemp(pattern);if(!name)return 2;const std::filesystem::path root(name);struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}} cleanup{root};try{
 const std::string policy(64,'a');fstr::AnalysisImage image{2,1,{120,70,40,255,180,120,90,255}};auto key=scene_fixture::write(root,image,1,policy);
 check(key=="1fb6347110921a37cc78bb10b794d2047d0c9660e048a988abde17abe5dc6f66","Cross-language cache key mismatch");
 fstr::SceneCache cache(root.string(),policy);auto first=cache.get(image,1);check(first&&first->people.size()==1&&first->people[0].samples.size()==40,"Scene cache did not load");check(first==cache.get(image,1),"RAM cache not shared");
 auto changed=image;changed.rgba[0]++;check(!cache.get(changed,1),"Changed upstream used old analysis");check(!cache.get(image,2),"Changed source scale used old analysis");fstr::SceneCache policy_changed(root.string(),std::string(64,'b'));check(!policy_changed.get(image,1),"Changed analysis policy used old data");check(cache.guid()!=policy_changed.guid(),"Policy absent from host cache GUID");
 fstr::SceneCache reopened(root.string(),policy);check(reopened.get(image,1)->scene==first->scene,"Fresh cache did not reopen disk");
 std::vector<std::future<std::shared_ptr<const fstr::SceneAnalysis>>> work;for(int i=0;i<8;++i)work.push_back(std::async(std::launch::async,[&]{return cache.get(image,1);}));for(auto&result:work)check(result.get()==first,"Parallel cache copies recomputed");
 auto bad=root/(key+"-p0.mask");{std::ofstream file(bad,std::ios::binary);file<<"bad";}cache.clear_memory();bool caught=false;try{cache.get(image,1);}catch(...){caught=true;}check(caught,"Corruption masquerades as valid analysis");
 scene_fixture::write(root,image,1,policy);auto header=root/(key+".fstrscene");nlohmann::json j;{std::ifstream f(header);f>>j;}j["people"][0]["samples"]=999999999;{std::ofstream f(header);f<<j.dump();}caught=false;try{cache.get(image,1);}catch(...){caught=true;}check(caught,"Oversized samples accepted");
 for(int n=0;n<6;++n){auto frame=image;frame.rgba[0]=static_cast<std::uint8_t>(10+n);scene_fixture::write(root,frame,1,policy);}
 fstr::SceneCache lru(root.string(),policy);std::weak_ptr<const fstr::SceneAnalysis> weak;
 for(int n=0;n<6;++n){auto frame=image;frame.rgba[0]=static_cast<std::uint8_t>(10+n);auto value=lru.get(frame,1);if(n==0)weak=value;}
 check(weak.expired(),"Scene RAM entry count is not bounded");auto fresh=image;fresh.rgba[0]=33;scene_fixture::write(root,fresh,1,policy);check(!lru.get(fresh,1),"Late ready record changed immutable snapshot");fstr::SceneCache newer(root.string(),policy);check(newer.guid()!=lru.guid()&&newer.get(fresh,1),"New snapshot not invalidated for host");

 auto stable_image=image;stable_image.rgba[0]=44;const auto stable_key=scene_fixture::write(root,stable_image,1,policy,true);fstr::SceneCache stable_cache(root.string(),policy);auto stable=stable_cache.get(stable_image,1);check(stable->people[0].toning_support==std::vector<float>(2,.75f)&&stable->people[0].toning_strength==.5,"Stable fields missing from cache");
 auto stable_header=root/(stable_key+".fstrscene");nlohmann::json sj;{std::ifstream f(stable_header);f>>sj;}sj["people"][0]["toning_strength"]=1.1;{std::ofstream f(stable_header);f<<sj.dump();}fstr::SceneCache bad_stable(root.string(),policy);caught=false;try{bad_stable.get(stable_image,1);}catch(...){caught=true;}check(caught,"Invalid stable strength accepted");
 std::cout<<"PASS scene cache integrity, invalidation, reopen and parallel sharing\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
