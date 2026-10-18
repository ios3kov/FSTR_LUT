#include "fstr/scene_window.hpp"
#include <algorithm>
#include <cmath>
#include <map>
namespace fstr {namespace {
double match_cost(const PersonAnalysis&a,const PersonAnalysis&b){auto center=[](const auto&v){return std::array<double,2>{(v.box[0]+v.box[2])*.5,(v.box[1]+v.box[3])*.5};};auto ca=center(a),cb=center(b);double ah=a.box[3]-a.box[1],bh=b.box[3]-b.box[1];if(ah<=0||bh<=0||a.detection_confidence<.8||b.detection_confidence<.8)return 1e30;double spatial=std::hypot(ca[0]-cb[0],ca[1]-cb[1])/std::max(ah,bh),scale=std::abs(std::log(ah/bh)),color=0;for(std::size_t c=0;c<3;++c)color+=std::pow(a.descriptor[c]-b.descriptor[c],2);color=std::sqrt(color);if(spatial>.6||scale>.5||color>.4)return 1e30;return spatial+.3*scale+.5*color;}
}
std::shared_ptr<const SceneAnalysis> stabilize_scene_window(const std::vector<AnalyzedFrame>&frames){
 std::map<int,std::shared_ptr<const SceneAnalysis>> window;for(const auto&f:frames){if(f.offset< -2||f.offset>2||!f.analysis||!window.emplace(f.offset,f.analysis).second)throw std::invalid_argument("Explicit scene window invalid");}auto center=window.find(0);if(center==window.end())throw std::invalid_argument("Current analysis missing");for(const auto&f:window)if(f.second->width!=center->second->width||f.second->height!=center->second->height)throw std::invalid_argument("Scene window geometry mismatch");auto result=std::make_shared<SceneAnalysis>(*center->second);
 for(std::size_t i=0;i<result->people.size();++i){auto&person=result->people[i];if(person.detection_confidence<.8)continue;std::vector<double> strengths{person.toning_strength};auto samples=person.samples;
  for(int offset:{-1,1}){auto it=window.find(offset);if(it==window.end())continue;const auto&others=it->second->people;double best=1e30,second=1e30;std::size_t index=0;for(std::size_t j=0;j<others.size();++j){double cost=match_cost(person,others[j]);if(cost<best){second=best;best=cost;index=j;}else second=std::min(second,cost);}if(best>=1e20||second-best<=.2)continue;bool unique=true;for(std::size_t k=0;k<result->people.size();++k)if(k!=i&&match_cost(result->people[k],others[index])-best<=.2){unique=false;break;}if(!unique)continue;strengths.push_back(others[index].toning_strength);const auto&more=others[index].samples;if(samples.size()+more.size()<=32768)samples.insert(samples.end(),more.begin(),more.end());}
  // Two-sided median suppresses one-frame confidence spikes. Never infer a
  // missing person's mask; one-sided source boundaries retain current strength.
  if(strengths.size()==3){std::sort(strengths.begin(),strengths.end());person.toning_strength=strengths[1];person.samples=std::move(samples);}
 }return result;
}
}
