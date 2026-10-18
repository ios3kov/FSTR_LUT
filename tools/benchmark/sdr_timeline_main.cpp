#include "fstr/sdr_timeline.hpp"
#include "fstr/sdr_prepare.hpp"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
using namespace fstr;
int main(int argc,char**argv){try{
 if(argc!=2)throw std::invalid_argument("Usage: fstr_sdr_timeline_cli manifest.json (complete ordered raw RGBA32F snapshots, max96x54)");
 if(std::filesystem::file_size(argv[1])>8*1024*1024)throw std::invalid_argument("Manifest budget exceeded");
 std::ifstream manifest(argv[1]);nlohmann::json j;manifest>>j;const auto w=j.at("width").get<std::size_t>(),h=j.at("height").get<std::size_t>();if(!w||w>96||!h||h>54)throw std::invalid_argument("Snapshot dimensions outside96x54");
 const auto name=j.at("transfer").get<std::string>();const auto t=name=="srgb"?Transfer::SRGB:name=="linear"?Transfer::Linear:name=="gamma24"?Transfer::Gamma24:throw std::invalid_argument("Unknown transfer");
 const auto files=j.at("files").get<std::vector<std::string>>();if(files.empty()||files.size()>sdr_timeline_frame_budget)throw std::invalid_argument("Timeline frame budget exceeded");
 auto read=[&](std::size_t i){const auto& path=files.at(i);const auto bytes=w*h*4*sizeof(float);if(std::filesystem::file_size(path)!=bytes)throw std::invalid_argument("Raw snapshot size differs");std::vector<float> v(w*h*4);std::ifstream f(path,std::ios::binary);f.read(reinterpret_cast<char*>(v.data()),std::streamsize(bytes));if(!f)throw std::runtime_error("Snapshot read failed");return v;};
 auto view=[&](const std::vector<float>&v){return FrameView{v.data(),v.size(),w,h,w*4,t,Primaries::SRGB_Rec709,Alpha::Premultiplied};};
 std::vector<SdrCutFeature> features;for(std::size_t i=0;i<files.size();++i){auto v=read(i);features.push_back(describe_sdr_cut_frame(view(v)));}auto shots=find_sdr_shots(features);nlohmann::json result{{"schema",1},{"frames",files.size()},{"shots",nlohmann::json::array()}};
 for(const auto&shot:shots){SdrShotPreparation a;if(shot.ambiguous)a.decision=SdrDecision::Ambiguous;else{std::vector<std::vector<float>> storage;for(auto i:std::set<std::size_t>(shot.anchors.begin(),shot.anchors.end()))storage.push_back(read(i));std::vector<FrameView> frames;for(const auto&v:storage)frames.push_back(view(v));a=analyze_sdr_shot(frames);}const auto&s=a.settings;const char* names[]{"prepared","balanced","ambiguous","inconsistent"};nlohmann::json recipe{{"schema",1},{"algorithm","sdr-shot-v2-prototype"},{"decision",names[int(a.decision)]},{"contrast",s.contrast},{"brightness_bias",s.brightness_bias},{"saturation",s.saturation},{"amount",s.amount}};result["shots"].push_back({{"begin",shot.begin},{"end",shot.end},{"anchors",shot.anchors},{"ambiguous",shot.ambiguous},{"recipe",recipe}});}
 std::cout<<result.dump(2)<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
