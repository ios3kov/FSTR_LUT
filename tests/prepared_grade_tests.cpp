#include "fstr/color.hpp"
#include <cstring>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace fstr::frozen {RGB grade_linear(RGB,const GradeSettings&);}
using namespace fstr;
struct Outcome {RGB value{};std::string error;bool operator==(const Outcome&o)const{return error==o.error&&std::memcmp(&value,&o.value,sizeof(value))==0;}};
template<class F>Outcome capture(F f){Outcome o;try{o.value=f();}catch(const std::exception&e){o.error=std::string(typeid(e).name())+":"+e.what();}return o;}
int main(){try{const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity(),huge=std::numeric_limits<float>::max();
 std::vector<RGB> pixels{{0,0,0},{-0.,0.,-0.},{-1,-.1,2},{huge,-huge,.5},{huge*2,0,0},{nan,.1,.2},{inf,.1,.2}};
 for(int i=0;i<100;++i)pixels.push_back({double(i)/99,double(i*31%100)/99,double(i*17%100)/99});
 std::size_t count=0;for(double exposure:{-10.,-.125,0.,.125,10.})for(double temp:{-1.,0.,.3,1.})for(double tint:{-1.,0.,.2,1.})for(double contrast:{.1,1.,1.7,3.})for(double saturation:{0.,1.,2.})for(bool natural:{false,true}){
  GradeSettings s;s.exposure_stops=exposure;s.temperature=temp;s.tint=tint;s.contrast=contrast;s.saturation=saturation;s.natural_contrast=natural;PreparedGrade prepared(s);
  for(auto p:pixels){const auto old=capture([&]{return frozen::grade_linear(p,s);});if(!(old==capture([&]{return prepared.apply(p);}))||!(old==capture([&]{return grade_linear(p,s);})))throw std::runtime_error("Prepared grade output/error differs");count+=2;}
 }
 for(int field=0;field<5;++field)for(double bad:{nan,inf,-inf,-11.,11.}){
  GradeSettings s;double* fields[]{&s.exposure_stops,&s.temperature,&s.tint,&s.contrast,&s.saturation};*fields[field]=bad;auto old=capture([&]{return frozen::grade_linear({nan,inf,huge*2},s);});auto now=capture([&]{return PreparedGrade(s).apply({nan,inf,huge*2});});if(!(old==now))throw std::runtime_error("Invalid settings/input error order");++count;
 }
 // Snapshot is independent of caller mutation and reusable out of order.
 GradeSettings s;s.exposure_stops=.5;PreparedGrade snapshot(s);const auto expected=snapshot.apply({.1,.2,.3});s.exposure_stops=9;auto actual=snapshot.apply({.1,.2,.3});if(std::memcmp(&expected,&actual,sizeof(RGB)))throw std::runtime_error("Mutable grading snapshot");
 std::cout<<"PASS "<<count<<" frozen grade bit/error comparisons; immutable snapshot\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
