#include "fstr/native_scene.hpp"
#include "fstr/automatic_scene_cache.hpp"
#include "json.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=4)throw std::invalid_argument("resources inputs-json new-output-dir");const auto root=std::filesystem::absolute(argv[3]);if(std::filesystem::exists(root))throw std::invalid_argument("Fresh output directory required");std::filesystem::create_directories(root);
 std::ifstream manifest(argv[2]);const auto records=nlohmann::json::parse(manifest);std::vector<fstr::AnalysisImage> inputs;std::vector<double> heights;
 if(!records.is_array()||records.empty()||records.size()>64)throw std::invalid_argument("Input list budget");
 for(const auto&record:records){fstr::AnalysisImage image;image.width=record.at("width");image.height=record.at("height");if(image.width<1||image.height<1||image.width>384||image.height>384)throw std::invalid_argument("Input shape");image.rgba.resize(std::size_t(image.width)*std::size_t(image.height)*4);std::ifstream f(record.at("path").get<std::string>(),std::ios::binary);f.read(reinterpret_cast<char*>(image.rgba.data()),std::streamsize(image.rgba.size()));if(!f||f.peek()!=EOF)throw std::runtime_error("Input bytes mismatch");inputs.push_back(std::move(image));heights.push_back(record.at("source_height"));}
 const auto begin=std::chrono::steady_clock::now();fstr::NativeSceneAnalyzer model(argv[1]);nlohmann::json report{{"initialization_ms",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count()},{"policy",model.policy()},{"cases",nlohmann::json::array()}};
 auto write=[](const std::filesystem::path&path,const void*data,std::size_t size){std::ofstream f(path,std::ios::binary);f.write(static_cast<const char*>(data),std::streamsize(size));if(!f)throw std::runtime_error("Output failed");};
 for(int pass=0;pass<4;++pass)for(int order=0;order<2;++order){const unsigned limit=1+unsigned((pass+order)%2);const auto dir=root/(std::to_string(pass)+"-"+std::to_string(limit));std::filesystem::create_directory(dir);std::atomic<int> computes{0};
 fstr::AutomaticSceneCache cache((dir/"cache").string(),model.policy(),[&](const auto&i,double h,const auto&cancel){++computes;return model.analyze(i,h,cancel);},{},limit);
 std::vector<std::future<std::shared_ptr<const fstr::SceneAnalysis>>> pending;const auto start=std::chrono::steady_clock::now();
 for(std::size_t i=0;i<inputs.size();++i)pending.push_back(std::async(std::launch::async,[&,i]{return cache.get(inputs[i],heights[i]);}));
 std::vector<std::shared_ptr<const fstr::SceneAnalysis>> results;for(auto&f:pending)results.push_back(f.get());const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();if(computes!=int(inputs.size()))throw std::runtime_error("Cold analysis count mismatch");
 nlohmann::json frames=nlohmann::json::array();for(std::size_t f=0;f<results.size();++f){const auto&a=*results[f];const auto stem=dir/std::to_string(f);write(stem.string()+".scene",a.scene.data(),a.scene.size()*sizeof(a.scene[0]));nlohmann::json people=nlohmann::json::array();for(std::size_t i=0;i<a.people.size();++i){const auto&p=a.people[i];auto name=stem.string()+"-p"+std::to_string(i);write(name+".mask",p.mask.data(),p.mask.size()*4);write(name+".support",p.toning_support.data(),p.toning_support.size()*4);std::vector<double> samples;for(const auto&s:p.samples)for(double v:{s.color.r,s.color.g,s.color.b,s.confidence})samples.push_back(v);write(name+".samples64",samples.data(),samples.size()*8);people.push_back({{"box",p.box},{"descriptor",p.descriptor},{"confidence",p.detection_confidence},{"strength",p.toning_strength},{"samples",p.samples.size()}});}frames.push_back({{"input",f},{"people",people}});}
 report["cases"].push_back({{"pass",pass},{"limit",limit},{"milliseconds",ms},{"computes",computes.load()},{"frames",frames}});std::ofstream out(root/"report.json");out<<report.dump(2);std::cout<<pass<<','<<limit<<','<<ms<<','<<computes<<std::endl;
 }
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
