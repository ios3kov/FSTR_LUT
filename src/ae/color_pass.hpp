#pragma once
#include "fstr/sdr_prepare.hpp"
#include "sdr_host_sample.hpp"
#include <cmath>
#include <memory>
namespace fstr::ae {
class PreparationError final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
// Render-local immutable settings. Captured input is encoded premultiplied RGB
// after the original float rounding and before the main look or final Mix.
class ColorPass final {
    fstr::ColorProcessor legacy_;
    std::unique_ptr<fstr::ColorProcessor> full_;
    fstr::SdrPreparation preparation_;
    fstr::Transfer transfer_;double mix_;bool share_prelook_;
public:
    ColorPass(fstr::ColorSettings s,fstr::Transfer transfer,fstr::Primaries primaries,const fstr::SdrPreparation* preparation)
        :legacy_(s,transfer,primaries,fstr::Alpha::Premultiplied),transfer_(transfer),mix_(s.mix),share_prelook_(s.source==fstr::SourceProfile::Host&&fstr::grade_is_neutral(s.grade)&&s.shoulder==0&&s.mix>0){
        if(preparation&&s.mix>0&&(preparation->contrast!=1||preparation->brightness_bias!=0||preparation->saturation!=1)){
            preparation_=*preparation;s.mix=1;full_=std::make_unique<fstr::ColorProcessor>(s,transfer,primaries,fstr::Alpha::Premultiplied);
        }
    }
    bool bypass()const noexcept{return !full_&&legacy_.bypass();}
    bool can_share_prelook()const noexcept{return share_prelook_;}
    // Capture requires can_share_prelook(); output is unspecified on exception.
    std::array<float,4> apply(std::array<float,4> raw,double protection=0,std::array<float,4>* normalized=nullptr)const{
        if(normalized&&!share_prelook_)throw std::logic_error("Prelook capture requires neutral Host preparation and positive Mix");
        if(!full_||raw[3]==0){if(normalized)*normalized=raw;return legacy_.apply(raw,protection);}
        const double alpha=raw[3];if(!std::isfinite(alpha)||alpha<=0||alpha>1)throw PreparationError("Invalid SDR alpha");
        const std::array<double,3> encoded{raw[0]/alpha,raw[1]/alpha,raw[2]/alpha};
        // Decode first, preserving transfer/nonfinite validation precedence.
        fstr::RGB base{fstr::decode_host(encoded[0],transfer_),fstr::decode_host(encoded[1],transfer_),fstr::decode_host(encoded[2],transfer_)};
        try{double* channels[]{&base.r,&base.g,&base.b};for(std::size_t c=0;c<3;++c)if(encoded[c]<0||encoded[c]>1)*channels[c]=fstr::decode_host(bounded_host_sdr(encoded[c]),transfer_);}catch(const std::invalid_argument&){throw PreparationError("Auto Prepare requires bounded SDR pixels");}
        fstr::RGB value;try{value=fstr::prepare_sdr_linear(base,preparation_);}catch(const std::invalid_argument&){throw PreparationError("Auto Prepare requires bounded SDR pixels");}
        const std::array<float,4> normalized_input{float(fstr::encode_host(value.r,transfer_)*alpha),float(fstr::encode_host(value.g,transfer_)*alpha),float(fstr::encode_host(value.b,transfer_)*alpha),raw[3]};
        if(normalized)*normalized=normalized_input;
        auto result=full_->apply(normalized_input,protection);
        if(mix_<1)for(std::size_t c=0;c<3;++c){const double source=fstr::decode_host(raw[c]/alpha,transfer_),styled=fstr::decode_host(result[c]/alpha,transfer_);result[c]=float(fstr::encode_host(source+mix_*(styled-source),transfer_)*alpha);}
        return result;
    }
};
}
