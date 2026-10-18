#include "fstr/sdr_timeline.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <limits>
namespace fstr { namespace {
struct Difference {double spatial=0,histogram=0,residual=0;};
Difference difference(const SdrCutFeature&a,const SdrCutFeature&b){
    Difference d;double aa=0,ab=0;
    for(std::size_t i=0;i<a.tiles.size();++i){d.spatial+=std::abs(a.tiles[i]-b.tiles[i]);aa+=a.tiles[i]*a.tiles[i];ab+=a.tiles[i]*b.tiles[i];}
    const double gain=aa>1e-8?std::clamp(ab/aa,.1,10.):1.;
    for(std::size_t i=0;i<a.tiles.size();++i)d.residual+=std::abs(b.tiles[i]-gain*a.tiles[i]);
    for(std::size_t i=0;i<a.histogram.size();++i)d.histogram+=std::abs(a.histogram[i]-b.histogram[i]);
    d.spatial/=double(a.tiles.size());d.residual/=double(a.tiles.size());d.histogram/=6.;return d;
}
}
SdrCutFeature describe_sdr_cut_frame(FrameView frame,const ColorContext&context){
    if(!frame.data||!frame.width||!frame.height||frame.width>16384||frame.height>16384||frame.row_stride<frame.width*4||frame.row_stride>100000000||frame.height-1>(std::numeric_limits<std::size_t>::max()-frame.width*4)/frame.row_stride||frame.count<(frame.height-1)*frame.row_stride+frame.width*4||frame.memory!=Memory::CPU||frame.primaries!=Primaries::SRGB_Rec709||(frame.transfer!=Transfer::SRGB&&frame.transfer!=Transfer::Linear&&frame.transfer!=Transfer::Gamma24))throw std::invalid_argument("Invalid SDR cut descriptor input");
    SdrCutFeature result;std::array<unsigned,32> counts{};unsigned visible=0;const auto nx=std::min<std::size_t>(64,frame.width),ny=std::min<std::size_t>(36,frame.height);
    for(std::size_t y=0;y<ny;++y){if(context.cancelled&&context.cancelled->load())throw std::runtime_error("Scene analysis cancelled");const auto sy=(2*y+1)*frame.height/(2*ny);
        for(std::size_t x=0;x<nx;++x){const auto sx=(2*x+1)*frame.width/(2*nx);const auto*p=frame.data+sy*frame.row_stride+sx*4;const float alpha=p[3];if(!std::isfinite(alpha)||alpha<0||alpha>1)throw std::invalid_argument("Invalid cut descriptor alpha");if(alpha<.5f)continue;const auto tile=std::min<std::size_t>(3,y*4/ny)*8+std::min<std::size_t>(7,x*8/nx);++counts[tile];++visible;
            for(std::size_t c=0;c<3;++c){double v=p[c];if(frame.alpha==Alpha::Premultiplied)v/=alpha;if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Scene detection requires SDR input");const double linear=decode_host(v,frame.transfer);result.tiles[tile*3+c]+=float(linear);const auto bin=std::min<std::size_t>(15,std::size_t(encode_host(linear,Transfer::SRGB)*16));result.histogram[c*16+bin]+=1;}
        }
    }
    for(std::size_t i=0;i<counts.size();++i)if(counts[i])for(std::size_t c=0;c<3;++c)result.tiles[i*3+c]/=float(counts[i]);
    if(visible)for(auto&v:result.histogram)v/=float(visible);result.coverage=float(visible)/float(nx*ny);return result;
}
std::vector<SdrShotRange> find_sdr_shots(const std::vector<SdrCutFeature>&frames,const ColorContext&context){
    if(frames.empty()||frames.size()>sdr_timeline_frame_budget)throw std::invalid_argument("Scene scan frame budget");
    for(const auto&f:frames){if(!std::isfinite(f.coverage)||f.coverage<0||f.coverage>1)throw std::invalid_argument("Invalid scene descriptor coverage");for(auto v:f.tiles)if(!std::isfinite(v)||v<0||v>1.00001f)throw std::invalid_argument("Invalid scene descriptor RGB");for(auto v:f.histogram)if(!std::isfinite(v)||v<0||v>1.00001f)throw std::invalid_argument("Invalid scene histogram");}
    std::vector<Difference> delta(frames.size());std::vector<bool> cut(frames.size()),unstable(frames.size()),gradual(frames.size());
    for(std::size_t i=1;i<frames.size();++i){if(context.cancelled&&context.cancelled->load())throw std::runtime_error("Scene analysis cancelled");delta[i]=difference(frames[i-1],frames[i]);const auto&d=delta[i];const bool visibility=std::abs(frames[i-1].coverage-frames[i].coverage)>.6;
        // A pure exposure jump preserves the scalar RGB fit. A histogram change
        // alone is insufficient: avoid cutting intentional lighting changes.
        cut[i]=visibility||(d.spatial>.025&&d.residual>.015&&(d.histogram>.28||d.spatial>.12));
    }
    // A cut can preserve a similar palette. An isolated structural residual
    // well above neighboring motion is additional evidence, independent of hue.
    for(std::size_t i=1;i<frames.size();++i){std::vector<double> neighbors;for(std::size_t j=i>2?i-2:1;j<std::min(frames.size(),i+3);++j)if(j!=i)neighbors.push_back(delta[j].residual);if(neighbors.size()>=2){std::sort(neighbors.begin(),neighbors.end());const auto&d=delta[i];if(d.spatial>.055&&d.residual>.04&&d.histogram>.1&&d.residual>4*std::max(.005,neighbors[neighbors.size()/2]))cut[i]=true;}}
    for(std::size_t i=1;i<frames.size();++i){std::vector<double> active;for(std::size_t j=i>2?i-2:1;j<std::min(frames.size(),i+3);++j)if(delta[j].spatial>.02)active.push_back(delta[j].spatial);if(active.size()>=3){std::sort(active.begin(),active.end());if(delta[i].spatial>.02&&delta[i].spatial<1.8*active[active.size()/2]){cut[i]=false;gradual[i]=true;}}}
    for(std::size_t i=1;i+1<frames.size();++i){const auto back=difference(frames[i-1],frames[i+1]);if(delta[i].spatial>.1&&delta[i+1].spatial>.1&&back.spatial<.035&&back.histogram<.12){cut[i]=cut[i+1]=false;unstable[i]=true;}}
    std::vector<SdrShotRange> result;std::size_t begin=0;
    auto append=[&](std::size_t end){SdrShotRange shot;shot.begin=begin;shot.end=end;for(std::size_t i=begin;i<end;++i)shot.ambiguous=shot.ambiguous||gradual[i];shot.anchors={begin,begin+(end-begin-1)/2,end-1};for(auto&anchor:shot.anchors){if(!unstable[anchor])continue;for(std::size_t offset=1;offset<end-begin;++offset){if(anchor>=begin+offset&&!unstable[anchor-offset]){anchor-=offset;break;}if(anchor+offset<end&&!unstable[anchor+offset]){anchor+=offset;break;}}}result.push_back(shot);begin=end;};
    for(std::size_t i=1;i<frames.size();++i)if(cut[i])append(i);append(frames.size());return result;
}
}
