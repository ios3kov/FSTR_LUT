// Frozen fecfcb8 ColorPass, plugin SHA in task source-binding.json. Independent numerical oracle.
namespace frozen {
using fstr::ae::PreparationError;
class BaselineColorPass final {
    fstr::ColorProcessor legacy_;
    std::unique_ptr<fstr::ColorProcessor> full_;
    fstr::SdrPreparation preparation_;
    fstr::Transfer transfer_;double mix_;
public:
    BaselineColorPass(fstr::ColorSettings s,fstr::Transfer transfer,fstr::Primaries primaries,const fstr::SdrPreparation* preparation)
        :legacy_(s,transfer,primaries,fstr::Alpha::Premultiplied),transfer_(transfer),mix_(s.mix){
        if(preparation&&s.mix>0&&(preparation->contrast!=1||preparation->brightness_bias!=0||preparation->saturation!=1)){
            preparation_=*preparation;s.mix=1;full_=std::make_unique<fstr::ColorProcessor>(s,transfer,primaries,fstr::Alpha::Premultiplied);
        }
    }
    bool bypass()const noexcept{return !full_&&legacy_.bypass();}
    std::array<float,4> apply(std::array<float,4> raw,double protection=0)const{
        if(!full_||raw[3]==0)return legacy_.apply(raw,protection);
        const double alpha=raw[3];if(!std::isfinite(alpha)||alpha<=0||alpha>1)throw PreparationError("Invalid SDR alpha");
        const fstr::RGB base{fstr::decode_host(raw[0]/alpha,transfer_),fstr::decode_host(raw[1]/alpha,transfer_),fstr::decode_host(raw[2]/alpha,transfer_)};
        fstr::RGB value;try{value=fstr::prepare_sdr_linear(base,preparation_);}catch(const std::invalid_argument&){throw PreparationError("Auto Prepare requires bounded SDR pixels");}
        auto result=full_->apply({float(fstr::encode_host(value.r,transfer_)*alpha),float(fstr::encode_host(value.g,transfer_)*alpha),float(fstr::encode_host(value.b,transfer_)*alpha),raw[3]},protection);
        if(mix_<1)for(std::size_t c=0;c<3;++c){const double source=fstr::decode_host(raw[c]/alpha,transfer_),styled=fstr::decode_host(result[c]/alpha,transfer_);result[c]=float(fstr::encode_host(source+mix_*(styled-source),transfer_)*alpha);}
        return result;
    }
};
}
