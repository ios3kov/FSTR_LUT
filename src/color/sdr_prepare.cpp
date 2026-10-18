#include "fstr/sdr_prepare.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
namespace fstr { namespace {
double bound(double x,double a,double b){return std::max(a,std::min(b,x));}
double luma(RGB c){return .2126*c.r+.7152*c.g+.0722*c.b;}
double logit(double x){x=bound(x,1e-6,1-1e-6);return std::log(x)-std::log1p(-x);}
double percentile(std::vector<double> v,double p){std::sort(v.begin(),v.end());return v[std::size_t(p*double(v.size()-1))];}
class PreparationCancelled final:public std::runtime_error {public:PreparationCancelled():std::runtime_error("SDR preparation cancelled"){};};
void abort(const ColorContext& c){if(c.cancelled&&c.cancelled->load())throw PreparationCancelled();}
void frame_bounds(FrameView f,const ColorContext& c){
    const auto limit=std::numeric_limits<std::size_t>::max();
    if(!f.data||!f.width||!f.height||f.width>16384||f.height>16384||f.width>c.max_pixels/f.height||
       f.row_stride<f.width*4||f.row_stride>limit/f.height||f.count<(f.height-1)*f.row_stride+f.width*4)
        throw std::invalid_argument("Invalid SDR preparation frame bounds");
    if(f.memory!=Memory::CPU||f.primaries!=Primaries::SRGB_Rec709||
       (f.transfer!=Transfer::SRGB&&f.transfer!=Transfer::Linear&&f.transfer!=Transfer::Gamma24)||
       (f.alpha!=Alpha::Straight&&f.alpha!=Alpha::Premultiplied))
        throw UnsupportedColorProfile("SDR preparation requires explicit CPU Rec709 samples");
}
RGB pixel(const float* p,FrameView f){
    const double a=p[3];if(!std::isfinite(a)||a<0||a>1)throw std::invalid_argument("Invalid SDR alpha");
    const double scale=f.alpha==Alpha::Premultiplied&&a>0?1/a:1;
    RGB c{p[0]*scale,p[1]*scale,p[2]*scale};
    for(double v:{c.r,c.g,c.b})if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("SDR preparation input is not bounded SDR");
    return {decode_host(c.r,f.transfer),decode_host(c.g,f.transfer),decode_host(c.b,f.transfer)};
}
}
void validate_sdr_preparation(const SdrPreparation& s){
    if(!std::isfinite(s.contrast)||s.contrast<1||s.contrast>1.8||!std::isfinite(s.brightness_bias)||std::abs(s.brightness_bias)>.45||
       !std::isfinite(s.saturation)||s.saturation<1||s.saturation>1.12||!std::isfinite(s.amount)||s.amount<0||s.amount>1)
        throw std::invalid_argument("Invalid SDR preparation settings");
}
SdrShotPreparation analyze_sdr_shot(const std::vector<FrameView>& frames,const ColorContext& context){
    if(frames.empty()||frames.size()>16)throw std::invalid_argument("SDR preparation requires1..16 shot anchors");
    SdrShotPreparation out;std::vector<double> values,chroma,medians,spans;bool ambiguous=false;
    for(const auto& f:frames){
        abort(context);frame_bounds(f,context);
        if(f.width!=frames[0].width||f.height!=frames[0].height)throw std::invalid_argument("SDR anchor geometry differs");
        const auto nx=std::min<std::size_t>(96,f.width),ny=std::min<std::size_t>(54,f.height);std::vector<double> local;
        for(std::size_t y=0;y<ny;++y){abort(context);for(std::size_t x=0;x<nx;++x){
            const auto p=f.data+((2*y+1)*f.height/(2*ny))*f.row_stride+((2*x+1)*f.width/(2*nx))*4;
            if(!std::isfinite(p[3])||p[3]<0||p[3]>1)throw std::invalid_argument("Invalid SDR alpha");
            if(p[3]<.5)continue;const auto c=pixel(p,f);const double v=luma(c);local.push_back(v);values.push_back(v);
            const double hi=std::max({c.r,c.g,c.b}),lo=std::min({c.r,c.g,c.b});
            if(hi>0&&v>.02&&v<.8)chroma.push_back((hi-lo)/hi);
        }}
        // Transparent/constant/low-key/high-key anchors are ambiguous, not evidence of a flat profile.
        if(local.size()<16){ambiguous=true;continue;}
        const double low=percentile(local,.05),high=percentile(local,.95);
        if(high-low<.025||high<.12||low>.35){ambiguous=true;continue;}
        medians.push_back(logit(percentile(local,.5)));spans.push_back(logit(high)-logit(low));
    }
    if(ambiguous)return out;
    if(*std::max_element(medians.begin(),medians.end())-*std::min_element(medians.begin(),medians.end())>.65||
       *std::max_element(spans.begin(),spans.end())-*std::min_element(spans.begin(),spans.end())>1.5){out.decision=SdrDecision::Inconsistent;return out;}
    out.samples=values.size();out.low=percentile(values,.05);out.median=percentile(values,.5);out.high=percentile(values,.95);
    const double span=logit(out.high)-logit(out.low);out.flatness=bound((5.2-span)/2,0,1);
    if(out.flatness<.05){out.decision=SdrDecision::Balanced;return out;}
    // Fit robust dark/light anchors instead of forcing the median to middle gray:
    // bright subjects/white uniforms must not be mistaken for overexposure.
    const double target_low=logit(.015),target_high=logit(.75);
    out.settings.contrast=1+(bound((target_high-target_low)/span,1,1.8)-1)*out.flatness;
    const double source_center=(logit(out.low)+logit(out.high))*.5;
    const double mapped_center=logit(.18)+out.settings.contrast*(source_center-logit(.18));
    out.settings.brightness_bias=bound((target_low+target_high)*.5-mapped_center,-.45,.45)*out.flatness;
    if(!chroma.empty()){
        const double q=percentile(chroma,.75);
        if(q>.05&&q<.7)out.settings.saturation=1+.12*out.flatness*bound((.7-q)/.5,0,1);
    }
    out.decision=SdrDecision::Prepared;return out;
}
RGB prepare_sdr_linear(RGB input,const SdrPreparation& s){
    validate_sdr_preparation(s);
    for(double v:{input.r,input.g,input.b})if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("Preparation requires linear SDR[0,1]");
    if(s.amount==0||(s.contrast==1&&s.brightness_bias==0&&s.saturation==1))return input;
    const double y=luma(input);if(y<=0||y>=1)return input;
    const double p=logit(.18),z=p+s.contrast*(std::log(y)-std::log1p(-y)-p)+s.brightness_bias;
    const double target=z>=0?1/(1+std::exp(-z)):std::exp(z)/(1+std::exp(z));
    RGB c{input.r*(target/y),input.g*(target/y),input.b*(target/y)};
    // One scalar chroma compression retains target luminance and linear RGB hue direction.
    double saturation=s.saturation;
    for(double v:{c.r,c.g,c.b}){
        const double d=v-target;
        if(d>0)saturation=std::min(saturation,(1-target)/d);
        else if(d<0)saturation=std::min(saturation,-target/d);
    }
    RGB result{target+(c.r-target)*saturation,target+(c.g-target)*saturation,target+(c.b-target)*saturation};
    for(double v:{result.r,result.g,result.b})if(v < -1e-12||v>1+1e-12)throw std::runtime_error("Preparation gamut bound failed");
    // Remove only floating-point boundary residue, after verifying the analytic bound.
    result={bound(result.r,0,1),bound(result.g,0,1),bound(result.b,0,1)};
    return {input.r+s.amount*(result.r-input.r),input.g+s.amount*(result.g-input.g),input.b+s.amount*(result.b-input.b)};
}
RenderResult render_sdr_preparation(FrameView f,const SdrPreparation& s,const ColorContext& context) noexcept{
    RenderResult out;
    try{
        frame_bounds(f,context);validate_sdr_preparation(s);abort(context);out.width=f.width;out.height=f.height;out.rgba.resize(f.width*f.height*4);
        const bool neutral=s.amount==0||(s.contrast==1&&s.brightness_bias==0&&s.saturation==1);
        for(std::size_t y=0;y<f.height;++y){abort(context);for(std::size_t x=0;x<f.width;++x){
            const auto p=f.data+y*f.row_stride+x*4;auto q=out.rgba.data()+(y*f.width+x)*4;
            if(neutral||p[3]==0){std::memcpy(q,p,4*sizeof(float));continue;}
            const auto c=prepare_sdr_linear(pixel(p,f),s);const double a=f.alpha==Alpha::Premultiplied?p[3]:1;
            q[0]=float(encode_host(c.r,f.transfer)*a);q[1]=float(encode_host(c.g,f.transfer)*a);q[2]=float(encode_host(c.b,f.transfer)*a);q[3]=p[3];
        }}out.status=Status::Ok;return out;
    }catch(const PreparationCancelled&){out.status=Status::Cancelled;}
    catch(const UnsupportedColorProfile&){out.status=Status::Unsupported;}
    catch(const std::invalid_argument&){out.status=Status::InvalidInput;}
    catch(...){out.status=Status::BackendError;}
    out.rgba.clear();return out;
}
}
