#include "fstr/sdr_timeline.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace fstr;
void check(bool v,const char*text){if(!v)throw std::runtime_error(text);}
std::vector<float> frame(int scene,double gain=1){std::vector<float> p(64*36*4);for(int y=0;y<36;y++)for(int x=0;x<64;x++){const auto i=std::size_t(y*64+x)*4;const bool tile=(scene==1?x>=32:x<32);p[i]=float(gain*(tile?.65:.08));p[i+1]=float(gain*(scene==2?.7:tile?.12:.42));p[i+2]=float(gain*(tile?.08:.6));p[i+3]=1;}return p;}
SdrCutFeature feature(const std::vector<float>&v){return describe_sdr_cut_frame({v.data(),v.size(),64,36,64*4,Transfer::Linear});}
int main(){try{
 const auto a=feature(frame(0)),b=feature(frame(1)),c=feature(frame(2));std::vector<SdrCutFeature> input;
 for(auto pair:{std::make_pair(0,7),std::make_pair(1,5),std::make_pair(2,8)})for(int i=0;i<pair.second;i++)input.push_back(pair.first==0?a:pair.first==1?b:c);
 auto shots=find_sdr_shots(input);check(shots.size()==3,"Known hard cuts not separated");check(shots[0].begin==0&&shots[0].end==7&&shots[1].begin==7&&shots[1].end==12&&shots[2].end==20,"Cut boundary off by one");
 for(const auto&s:shots)for(auto i:s.anchors)check(i>=s.begin&&i<s.end,"Anchor crosses cut");check(find_sdr_shots(std::vector<SdrCutFeature>(20,a)).size()==1,"Static scene cut");
 // Same channel histograms, different spatial arrangement: a versus b.
 check(a.histogram==b.histogram,"Spatial fixture must retain its histogram");check(find_sdr_shots({a,a,b,b}).size()==2,"Histogram-only detector missed spatial cut");
 // Moderate spatial changes with similar palettes still split when isolated.
 auto subtle=frame(0);for(std::size_t i=0;i<subtle.size();i+=4){std::swap(subtle[i],subtle[i+2]);subtle[i]=.7f*subtle[i]+.3f*frame(0)[i];}const auto mild=feature(subtle);check(find_sdr_shots({a,a,a,mild,mild,mild}).size()==2,"Isolated similar-palette cut missed");
 input={feature(frame(0,.3)),feature(frame(0,.5)),feature(frame(0,.8)),feature(frame(0,1))};check(find_sdr_shots(input).size()==1,"Scalar exposure changes split a scene");
 input=std::vector<SdrCutFeature>(9,feature(frame(0,.4)));input[4]=feature(frame(0,1));shots=find_sdr_shots(input);check(shots.size()==1,"Returning flash split a scene");for(auto i:shots[0].anchors)check(i!=4,"Flash chosen as color anchor");
 shots=find_sdr_shots({a,b,b,c});check(shots.size()==3&&shots[0].end==1&&shots.back().begin==3,"Boundary/short shot missing");
 auto original=find_sdr_shots(std::vector<SdrCutFeature>{a,a,b,b,c,c});for(int i:{5,0,4,1,3,2}){auto again=find_sdr_shots({a,a,b,b,c,c});std::size_t index=0;while(std::size_t(i)>=again[index].end)++index;check(again[index].begin==original[index].begin&&again[index].anchors==original[index].anchors,"Request order changed shot plan");}
 input.clear();auto fa=frame(0),fb=frame(1);for(int step=0;step<12;++step){auto f=fa;for(std::size_t i=0;i<f.size();++i)f[i]=float(fa[i]+(fb[i]-fa[i])*step/11.);input.push_back(feature(f));}shots=find_sdr_shots(input);check(shots.size()==1&&shots[0].ambiguous,"Gradual transition got unstable per-frame grades");
 auto v=frame(0);v[0]=std::numeric_limits<float>::infinity();bool refused=false;try{feature(v);}catch(const std::invalid_argument&){refused=true;}check(refused,"Nonfinite SDR accepted");
 auto f=a;f.tiles[0]=std::numeric_limits<float>::quiet_NaN();refused=false;try{find_sdr_shots({f});}catch(const std::invalid_argument&){refused=true;}check(refused,"Invalid descriptors accepted");
 std::atomic_bool cancelled{true};ColorContext ctx;ctx.cancelled=&cancelled;refused=false;try{find_sdr_shots({a,b},ctx);}catch(const std::runtime_error&){refused=true;}check(refused,"Scene cancellation ignored");
 refused=false;try{find_sdr_shots({});}catch(const std::invalid_argument&){refused=true;}check(refused,"Empty complete scan accepted");
 std::cout<<"PASS scene boundaries, spatial-only cuts, exposure/flash, anchors, order, invalid inputs and cancellation\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
