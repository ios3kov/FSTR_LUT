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
std::vector<ColorRegion> recover_color_regions(const std::vector<RegionFrame>& frames){
 if(frames.empty()||frames.size()>5)throw std::invalid_argument("Invalid region window");
 const RegionFrame* ordered[5]{};
 for(const auto&f:frames){
  if(f.offset< -2||f.offset>2||ordered[f.offset+2])throw std::invalid_argument("Invalid frame offset");
  const auto&i=f.image;if(i.width<1||i.height<1||i.width>2048||i.height>2048||i.rgba.size()!=std::size_t(i.width)*i.height*4)throw std::invalid_argument("Invalid window image");
  if(f.regions.size()>16)throw std::invalid_argument("Too many regions");
  for(const auto&r:f.regions)if(!valid(r))throw std::invalid_argument("Invalid seed");
  ordered[f.offset+2]=&f;
 }
 if(!ordered[2])throw std::invalid_argument("Current frame absent");
 const auto&current=*ordered[2];auto out=current.regions;
 for(const auto&f:frames)if(f.image.width!=current.image.width||f.image.height!=current.image.height)throw std::invalid_argument("Window geometry mismatch");
 // Low-frequency appearance is appropriate for blurred color objects. Average
 // all pixels in each of nine cells, rather than matching facial texture.
 auto signature=[](const AnalysisImage&i,const ColorRegion&r){std::array<double,27> values{};
  for(int gy=0;gy<3;++gy)for(int gx=0;gx<3;++gx){
   int x0=std::clamp(int((r.x0+(r.x1-r.x0)*gx/3)*i.width),0,i.width-1),x1=std::clamp(int(std::ceil((r.x0+(r.x1-r.x0)*(gx+1)/3)*i.width)),x0+1,i.width);
   int y0=std::clamp(int((r.y0+(r.y1-r.y0)*gy/3)*i.height),0,i.height-1),y1=std::clamp(int(std::ceil((r.y0+(r.y1-r.y0)*(gy+1)/3)*i.height)),y0+1,i.height);
   for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x)for(int c=0;c<3;++c)values[(gy*3+gx)*3+c]+=i.rgba[(std::size_t(y)*i.width+x)*4+c];
   for(int c=0;c<3;++c)values[(gy*3+gx)*3+c]/=255.*(x1-x0)*(y1-y0);
  }return values;
 };
 auto distance=[](const auto&a,const auto&b){double e=0;for(std::size_t i=0;i<a.size();++i)e+=std::abs(a[i]-b[i]);return e/a.size();};
 auto overlap=[](const ColorRegion&a,const ColorRegion&b){return std::min(a.x1,b.x1)>std::max(a.x0,b.x0)&&std::min(a.y1,b.y1)>std::max(a.y0,b.y0);};
 for(int li:{1,0})for(int ri:{3,4}){
  // At most two missing frames, both endpoints explicitly detected.
  if(ri-li>3||!ordered[li]||!ordered[ri])continue;
  bool continuous=true;for(int k=li;k<ri;++k){if(!ordered[k]||!ordered[k+1]){continuous=false;break;}ColorRegion whole{0,0,1,1,0,{}};if(distance(signature(ordered[k]->image,whole),signature(ordered[k+1]->image,whole))>.10){continuous=false;break;}}
  if(!continuous)continue;
  const auto&left=*ordered[li];const auto&right=*ordered[ri];
  for(const auto&a:left.regions){
   const auto*b=match(a,right.regions);if(!b||match(*b,left.regions)!=&a)continue;
   const double t=double(2-li)/(ri-li);ColorRegion target{a.x0+(b->x0-a.x0)*t,a.y0+(b->y0-a.y0)*t,a.x1+(b->x1-a.x1)*t,a.y1+(b->y1-a.y1)*t,0,{}};
   bool competing=false;for(const auto&r:out)if(overlap(target,r)){competing=true;break;}if(competing)continue;
   const auto appearance_left=signature(left.image,a),appearance_right=signature(right.image,*b);
   // Each missing sample must match both anchors. A bounded local search allows
   // small non-linear motion around the time-interpolated position.
   bool verified=true;for(int k=li+1;k<ri;++k){const double u=double(k-li)/(ri-li);ColorRegion predicted{a.x0+(b->x0-a.x0)*u,a.y0+(b->y0-a.y0)*u,a.x1+(b->x1-a.x1)*u,a.y1+(b->y1-a.y1)*u,0,{}};
    for(const auto&r:ordered[k]->regions)if(overlap(predicted,r)){verified=false;break;}
    if(!verified)break;
    double best=1e9,selected_error=1e9;ColorRegion selected;
    for(int dy=-4;dy<=4;++dy)for(int dx=-4;dx<=4;++dx){auto p=predicted;double sx=dx*(p.x1-p.x0)/16,sy=dy*(p.y1-p.y0)/16;p.x0+=sx;p.x1+=sx;p.y0+=sy;p.y1+=sy;if(!valid(p))continue;
     bool occupied=false;for(const auto&r:ordered[k]->regions)if(overlap(p,r)){occupied=true;break;}if(occupied)continue;
     auto appearance=signature(ordered[k]->image,p);double error=std::max(distance(appearance_left,appearance),distance(appearance_right,appearance));
     // Tie-break toward the interpolated location; appearance gate unchanged.
     double score=error+1e-7*(dx*dx+dy*dy);if(score<best){best=score;selected_error=error;selected=p;}
    }
    if(selected_error>.08){verified=false;break;}if(k==2)target=selected;
   }
   for(const auto&r:out)if(overlap(target,r)){verified=false;break;}
   if(verified&&out.size()<16)out.push_back(std::move(target));
  }
 }
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
