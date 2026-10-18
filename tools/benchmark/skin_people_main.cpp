// Research frame job. JSON paths identify explicit automatic analysis outputs.
#include "fstr/scene_color.hpp"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::invalid_argument("people-frame-job.json");std::ifstream config(argv[1]);auto j=nlohmann::json::parse(config);
 const int w=j.at("width"),h=j.at("height");if(w<1||h<1||w>8192||h>8192||std::size_t(w)*h>8388608)throw std::invalid_argument("Frame budget");
 const bool enabled=j.value("enabled",true);
 const auto people=enabled?j.at("people"):nlohmann::json::array();if(!people.is_array()||people.size()>16)throw std::invalid_argument("Person budget");
 const std::size_t pixels=std::size_t(w)*h;const std::string input_path=j.at("input"),output_path=j.at("output"),lut_path=j.at("lut");
 if(std::filesystem::exists(output_path))throw std::invalid_argument("Output exists");if(std::filesystem::file_size(input_path)!=pixels*16)throw std::invalid_argument("Frame size");
 std::ifstream cube(lut_path);auto lut=std::make_shared<fstr::LUT>(fstr::LUT::parse(cube));fstr::SceneLook grade(lut,j.value("strength",1.));
 std::vector<fstr::SkinToneReference> references;std::vector<std::ifstream> masks,supports;std::vector<fstr::StableToneWeight> stable;references.reserve(people.size());masks.reserve(people.size());
 for(const auto&person:people){const std::string samples_path=person.at("samples"),mask_path=person.at("mask");const auto bytes=std::filesystem::file_size(samples_path);if(bytes%16||bytes/16>1048576||std::filesystem::file_size(mask_path)!=pixels*4)throw std::invalid_argument("Analysis size");
  std::ifstream source(samples_path,std::ios::binary);std::vector<fstr::SkinSample> samples;
  for(std::uintmax_t i=0;i<bytes/16;++i){std::array<float,4> v{};source.read(reinterpret_cast<char*>(v.data()),sizeof(v));if(!source)throw std::runtime_error("Samples read");samples.push_back({{v[0],v[1],v[2]},v[3]});}
  references.push_back(grade.skin_reference(samples));masks.emplace_back(mask_path,std::ios::binary);stable.push_back({0,person.value("toning_strength",0.)});if(person.contains("toning_support")){const std::string path=person.at("toning_support");if(std::filesystem::file_size(path)!=pixels*4)throw std::invalid_argument("Stable support size");supports.emplace_back(path,std::ios::binary);}else supports.emplace_back();
 }
 std::vector<fstr::SkinToneInfluence> influences(people.size());for(std::size_t i=0;i<people.size();++i)influences[i].reference=&references[i];
 const bool has_scene=enabled&&j.contains("scene");std::ifstream scene;
 if(has_scene){const std::string path=j.at("scene");if(std::filesystem::file_size(path)!=pixels*sizeof(fstr::SceneWeights))throw std::invalid_argument("Scene size");scene.open(path,std::ios::binary);}
 std::ifstream input(input_path,std::ios::binary);std::vector<std::array<float,4>> result;result.reserve(pixels);
 for(std::size_t i=0;i<pixels;++i){std::array<float,4> p{};input.read(reinterpret_cast<char*>(p.data()),sizeof(p));if(!input)throw std::runtime_error("Frame read");
  for(std::size_t k=0;k<people.size();++k){float mask=0;masks[k].read(reinterpret_cast<char*>(&mask),sizeof(mask));if(!masks[k])throw std::runtime_error("Mask read");influences[k].weight=mask;if(supports[k].is_open()){float value=0;supports[k].read(reinterpret_cast<char*>(&value),sizeof(value));if(!supports[k])throw std::runtime_error("Support read");stable[k].support=value;}}
  fstr::SceneWeights weights{};if(has_scene){scene.read(reinterpret_cast<char*>(weights.data()),sizeof(weights));if(!scene)throw std::runtime_error("Scene read");}else for(const auto&v:influences)weights[0]+=float(v.weight);
  result.push_back(grade.apply_stable_people(p,weights,influences,stable,enabled));
 }
 std::ofstream output(output_path,std::ios::binary);for(const auto&p:result)output.write(reinterpret_cast<const char*>(p.data()),sizeof(p));output.close();if(!output)throw std::runtime_error("Output write");return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
