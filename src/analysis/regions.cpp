#include "fstr/regions.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace fstr {
namespace {
double smooth(double a,double b,double x){x=std::clamp((x-a)/(b-a),0.,1.);return x*x*(3-2*x);}
bool valid(const ColorRegion&r){return std::isfinite(r.x0)&&std::isfinite(r.y0)&&std::isfinite(r.x1)&&std::isfinite(r.y1)&&r.x0>=0&&r.y0>=0&&r.x1<=1&&r.y1<=1&&r.x1>r.x0&&r.y1>r.y0&&std::isfinite(r.detail)&&r.detail>=0&&r.detail<=1;}
const ColorRegion* match(const ColorRegion&a,const std::vector<ColorRegion>& v){
 const ColorRegion* found=nullptr;
 for(const auto&b:v){const double h=a.y1-a.y0,k=b.y1-b.y0,w=a.x1-a.x0,z=b.x1-b.x0;
  if(k/h<.65||k/h>1.5||z/w<.65||z/w>1.5)continue;
  if(std::hypot((a.x0+a.x1-b.x0-b.x1)/2,(a.y0+a.y1-b.y0-b.y1)/2)>.5*std::min(h,k))continue;
  if(found)return nullptr;found=&b;}
 return found;
}
bool inside(double x,double y,const std::vector<std::array<double,2>>&p){bool c=false;if(p.size()<3)return false;for(std::size_t i=0,j=p.size()-1;i<p.size();j=i++)if((p[i][1]>y)!=(p[j][1]>y)&&x<(p[j][0]-p[i][0])*(y-p[i][1])/(p[j][1]-p[i][1])+p[i][0])c=!c;return c;}
double hole_weight(double x,double y,const std::vector<std::array<double,2>>& p,double feather){
 if(!inside(x,y,p))return 0;
 double distance=1;
 for(std::size_t i=0,j=p.size()-1;i<p.size();j=i++){
  const double dx=p[i][0]-p[j][0],dy=p[i][1]-p[j][1],length=dx*dx+dy*dy;
  const double t=length>0?std::clamp(((x-p[j][0])*dx+(y-p[j][1])*dy)/length,0.,1.):0;
  distance=std::min(distance,std::hypot(x-p[j][0]-t*dx,y-p[j][1]-t*dy));
 }return smooth(0,feather,distance);
}

}
std::vector<ColorRegion> stabilize_color_regions(const std::vector<ColorRegion>& current,const std::vector<ColorRegion>& before,const std::vector<ColorRegion>& after){
 for(const auto*v:{&current,&before,&after}){if(v->size()>16)throw std::invalid_argument("Too many regions");for(const auto&r:*v)if(!valid(r))throw std::invalid_argument("Invalid region");}
 auto out=current;
 for(std::size_t i=0;i<out.size();++i){const auto*l=match(current[i],before);const auto*r=match(current[i],after);
  if(!l||!r||match(*l,current)!=&current[i]||match(*r,current)!=&current[i])continue;
  auto&v=out[i];v.x0=.75*v.x0+.125*(l->x0+r->x0);v.y0=.75*v.y0+.125*(l->y0+r->y0);v.x1=.75*v.x1+.125*(l->x1+r->x1);v.y1=.75*v.y1+.125*(l->y1+r->y1);
  v.detail=std::min(v.detail,.75*v.detail+.125*(l->detail+r->detail));
 }
 // No guessed gap recovery in native adapter until a photometric gate exists.
 return out;
}
double color_region_weight(double x,double y,double source_height,const std::vector<ColorRegion>&regions){
 double weight=0;
 for(const auto&r:regions){const double w=r.x1-r.x0,h=r.y1-r.y0;
  const double radius=std::hypot((x-(r.x0+r.x1)/2)/(.47*w),(y-(r.y0+r.y1)/2)/(.47*h));
  double v=.65*(1-smooth(.65,1.,radius))*smooth(24,32,h*source_height);
  const double detail=.5*smooth(64,192,h*source_height)*r.detail;
  double exclusion=0;for(const auto&hole:r.holes)exclusion=std::max(exclusion,hole_weight(x,y,hole,.04*std::min(w,h)));
  v*=1-detail*exclusion;
  weight=std::max(weight,v);
 }return weight;
}
}
