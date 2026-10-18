#include "fstr/external_lut.hpp"
#include "picosha2.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <cerrno>
#include <cctype>
#include <sstream>
#include "tinyxml2.h"
#ifdef FSTR_EXTERNAL_LUT
#include <OpenColorIO/OpenColorIO.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace fstr {
namespace {
std::string trim(std::string s){const auto a=s.find_first_not_of(" \r\n\t\"'");if(a==std::string::npos)return {};return s.substr(a,s.find_last_not_of(" \r\n\t\"'")-a+1);}
std::size_t dimension(const std::string& text,std::size_t limit,bool zero=false){const auto s=trim(text);std::size_t at=0;const auto n=std::stoull(s,&at);if(at!=s.size()||n>limit||(!zero&&n<2))throw std::invalid_argument("LUT dimension budget exceeded");return std::size_t(n);}
void preflight(const std::string& bytes,const std::string& ext){
    if(ext==".look"){
        tinyxml2::XMLDocument xml;if(xml.Parse(bytes.data(),bytes.size())!=tinyxml2::XML_SUCCESS)throw std::invalid_argument("Invalid LOOK XML");const auto* root=xml.FirstChildElement("look");const auto* lut=root?root->FirstChildElement("LUT"):nullptr;const auto* size=lut?lut->FirstChildElement("size"):nullptr;if(!size||!size->GetText())throw std::invalid_argument("Missing LOOK grid");dimension(size->GetText(),65);return;
    }
    std::istringstream stream(bytes);stream.imbue(std::locale::classic());std::string line;
    auto next=[&](){while(std::getline(stream,line)){if(auto p=line.find('#');p!=std::string::npos)line.resize(p);line=trim(line);if(!line.empty())return line;}throw std::invalid_argument("Truncated LUT header");};
    if(ext==".csp"){
        if(next()!="CSPLUTV100")throw std::invalid_argument("Invalid CSP header");const auto type=next();if(type!="1D"&&type!="3D")throw std::invalid_argument("Unsupported CSP dimension");next();
        if(line=="BEGIN METADATA"){do{next();}while(line!="END METADATA");next();}
        for(int c=0;c<3;++c){const auto count=dimension(line,65536,true);if(count>=2){next();next();}next();}
        if(type=="1D")dimension(line,65536);else {std::istringstream sizes(line);std::string a,b,c,extra;if(!(sizes>>a>>b>>c)||(sizes>>extra)||dimension(a,65)!=dimension(b,65)||dimension(a,65)!=dimension(c,65))throw std::invalid_argument("Invalid CSP cube size");}return;
    }
    std::size_t triples=0;
    while(std::getline(stream,line)){
        if(auto p=line.find('#');p!=std::string::npos)line.resize(p);std::istringstream row(line);std::string tag;if(!(row>>tag))continue;
        if(ext==".cube"&&(tag=="LUT_3D_SIZE"||tag=="LUT_1D_SIZE")){std::string n;if(!(row>>n))throw std::invalid_argument("Missing CUBE dimension");dimension(n,tag=="LUT_3D_SIZE"?65:65536);}
        if(ext==".3dl"){std::istringstream numeric(line);double value;std::size_t count=0;while(numeric>>value)if(++count>65536)throw std::invalid_argument("3DL shaper budget exceeded");if(count==3&&++triples>65*65*65)throw std::invalid_argument("3DL grid budget exceeded");}
    }
}
}
ExternalLUT load_external_lut(const std::string& path){
    if(path.empty()||path.size()>4095||path.find('\0')!=std::string::npos||!std::filesystem::path(path).is_absolute())throw std::invalid_argument("Choose an absolute external LUT file");
    auto ext=std::filesystem::path(path).extension().string();std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(ext!=".cube"&&ext!=".3dl"&&ext!=".look"&&ext!=".csp")throw std::invalid_argument("Choose .cube, .3dl, .look or .csp");
#ifdef FSTR_EXTERNAL_LUT
    namespace ocio=OCIO_NAMESPACE;
    static std::mutex loading;std::lock_guard<std::mutex> guard(loading);
    constexpr std::size_t max_bytes=32*1024*1024;
    struct File {int fd=-1;std::string temporary;~File(){if(fd>=0)::close(fd);if(!temporary.empty())::unlink(temporary.c_str());}} input,snapshot;
    input.fd=::open(path.c_str(),O_RDONLY|O_CLOEXEC|O_NONBLOCK);struct stat status{};
    if(input.fd<0||::fstat(input.fd,&status)||!S_ISREG(status.st_mode)||status.st_size<1||std::uint64_t(status.st_size)>max_bytes)throw std::runtime_error("LUT file unavailable, empty or over 32 MiB");
    std::string bytes;bytes.reserve(std::size_t(status.st_size));char buffer[16384];
    for(;;){const auto n=::read(input.fd,buffer,sizeof(buffer));if(n<0&&errno==EINTR)continue;if(n<0)throw std::runtime_error("Cannot read LUT");if(n==0)break;if(bytes.size()+std::size_t(n)>max_bytes)throw std::runtime_error("LUT exceeds 32 MiB");bytes.append(buffer,std::size_t(n));}
    preflight(bytes,ext);
    // Parse immutable bytes, avoiding content/hash races and OCIO's pathname cache.
    std::string pattern=(std::filesystem::temp_directory_path()/("fstr-lut-XXXXXX"+ext)).string();std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
    snapshot.fd=::mkstemps(name.data(),int(ext.size()));if(snapshot.fd<0)throw std::runtime_error("Cannot prepare LUT import");snapshot.temporary=name.data();
    std::size_t written=0;while(written<bytes.size()){const auto n=::write(snapshot.fd,bytes.data()+written,bytes.size()-written);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("Cannot prepare LUT import");written+=std::size_t(n);}
    ::close(snapshot.fd);snapshot.fd=-1;
    // This private OCIO namespace/cache is isolated from other AE plug-ins.
    struct Clear {~Clear(){try{ocio::ClearAllCaches();}catch(...) {}}} clear;
    const auto config=ocio::Config::CreateRaw();auto transform=ocio::FileTransform::Create();transform->setSrc(snapshot.temporary.c_str());transform->setInterpolation(ocio::INTERP_TETRAHEDRAL);
    const auto processor=config->getProcessor(transform);const auto group=processor->createGroupTransform();
    if(group->getNumTransforms()>16)throw std::invalid_argument("LUT operation budget exceeded");
    auto finite=[](float a,float b,float c){if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c))throw std::invalid_argument("Nonfinite LUT values");};
    for(int i=0;i<group->getNumTransforms();++i){const auto op=group->getTransform(i);op->validate();
        if(op->getTransformType()==ocio::TRANSFORM_TYPE_LUT3D){auto lut=std::dynamic_pointer_cast<const ocio::Lut3DTransform>(op);const auto n=lut->getGridSize();if(n>65)throw std::invalid_argument("LUT grid larger than 65");for(unsigned long r=0;r<n;++r)for(unsigned long g=0;g<n;++g)for(unsigned long b=0;b<n;++b){float x,y,z;lut->getValue(r,g,b,x,y,z);finite(x,y,z);}}
        if(op->getTransformType()==ocio::TRANSFORM_TYPE_LUT1D){auto lut=std::dynamic_pointer_cast<const ocio::Lut1DTransform>(op);const auto n=lut->getLength();if(n>65536)throw std::invalid_argument("LUT shaper larger than 65536");for(unsigned long j=0;j<n;++j){float x,y,z;lut->getValue(j,x,y,z);finite(x,y,z);}}
    }
    const auto cpu=processor->getOptimizedCPUProcessor(ocio::OPTIMIZATION_DEFAULT);
    auto sample=[cpu](RGB in){for(double v:{in.r,in.g,in.b})if(std::abs(v)>std::numeric_limits<float>::max())throw std::invalid_argument("LUT input exceeds float range");float rgb[]{float(in.r),float(in.g),float(in.b)};cpu->applyRGB(rgb);return RGB{rgb[0],rgb[1],rgb[2]};};
    ExternalLUT result;result.look=std::make_shared<const LUT>(LUT::from_sampler(std::move(sample)));result.bytes=bytes.size();result.identity=picosha2::hash256_hex_string("FSTR_EXTERNAL_OCIO_V1/2.4.2/TETRA\n"+ext+"\n"+bytes);return result;
#else
    throw std::runtime_error("External LUT support is unavailable in this build");
#endif
}
}
