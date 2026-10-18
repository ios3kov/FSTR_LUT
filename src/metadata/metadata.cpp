#include "fstr/metadata.hpp"
#include "tinyxml2.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <functional>
#include <limits>

namespace fstr {
namespace {
CameraMetadata failure(MetadataStatus status,const char* message) {
    CameraMetadata result;result.status=status;result.message=message;return result;
}
std::string_view local_name(const char* name) {
    if(!name)return {};
    const std::string_view value(name);const auto colon=value.find(':');
    return colon==std::string_view::npos?value:value.substr(colon+1);
}
std::string_view qualified_namespace(const tinyxml2::XMLElement* element,std::string_view name,bool is_attribute=false) {
    const auto colon=name.find(':');if(is_attribute&&colon==std::string_view::npos)return {};
    const std::string attribute=colon==std::string_view::npos?"xmlns":"xmlns:"+std::string(name.substr(0,colon));
    for(auto* node=element;node;node=node->Parent()?node->Parent()->ToElement():nullptr)
        if(const auto* value=node->Attribute(attribute.c_str()))return value;
    return {};
}
std::string_view element_namespace(const tinyxml2::XMLElement* e){return qualified_namespace(e,e->Name());}
bool assign(std::string& field,const char* value,bool& conflict) {
    if(!value)return true;
    const std::string_view text(value);
    if(text.size()>128||std::any_of(text.begin(),text.end(),[](unsigned char c){return c<32||c==127;}))return false;
    if(!field.empty()&&field!=text)conflict=true;
    else field=std::string(text);
    return true;
}
std::string normalized(const std::string& text) {
    std::string out;
    for(unsigned char c:text)if(c<128&&std::isalnum(c))out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}
std::uint64_t big_endian(const unsigned char* bytes,unsigned count) {
    std::uint64_t value=0;for(unsigned i=0;i<count;++i)value=(value<<8)|bytes[i];return value;
}
bool merge(CameraMetadata& a,const CameraMetadata& b) {
    bool conflict=false;
    for(auto pair:{std::make_pair(&a.manufacturer,&b.manufacturer),std::make_pair(&a.model,&b.model),std::make_pair(&a.gamma,&b.gamma),std::make_pair(&a.primaries,&b.primaries),std::make_pair(&a.coding,&b.coding)})
        if(!pair.second->empty()&&!assign(*pair.first,pair.second->c_str(),conflict))return false;
    if(conflict)return false;
    if(a.profile==DeclaredProfile::Unknown)a.profile=b.profile;
    if(a.provenance.empty())a.provenance=b.provenance;else if(a.provenance.find(b.provenance)==std::string::npos)a.provenance+=" + "+b.provenance;
    a.status=MetadataStatus::Found;a.message="Source declarations; AE or prior effects may already have converted the pixels";return true;
}
}
CameraMetadata parse_sony_metadata_xml(std::string_view xml,MetadataLimits limits) {
    if(xml.size()>limits.xml_bytes)return failure(MetadataStatus::LimitExceeded,"Metadata XML exceeds byte budget");
    if(xml.find('\0')!=std::string_view::npos||xml.find("<!DOCTYPE")!=std::string_view::npos||xml.find("<!ENTITY")!=std::string_view::npos)
        return failure(MetadataStatus::Invalid,"Unsupported XML declaration or binary data");
    tinyxml2::XMLDocument document;
    if(document.Parse(xml.data(),xml.size())!=tinyxml2::XML_SUCCESS)return failure(MetadataStatus::Invalid,"Invalid camera XML");
    const auto* root=document.RootElement();
    if(!root||local_name(root->Name())!="NonRealTimeMeta")return failure(MetadataStatus::Unsupported,"Not Sony NonRealTimeMeta");
    if(root->NextSiblingElement())return failure(MetadataStatus::Ambiguous,"Multiple metadata document roots");
    const auto ns=element_namespace(root);
    if(ns.find("urn:schemas-professionalDisc:nonRealTimeMeta:ver.")!=0)
        return failure(MetadataStatus::Unsupported,"Unknown camera XML namespace");
    CameraMetadata result;bool conflict=false;
    for(auto* child=root->FirstChildElement();child;child=child->NextSiblingElement()) {
        if((local_name(child->Name())=="Device"||local_name(child->Name())=="AcquisitionRecord")&&element_namespace(child)!=ns)
            return failure(MetadataStatus::Invalid,"Foreign namespace in camera metadata");
        if(local_name(child->Name())=="Device") {
            if(!assign(result.manufacturer,child->Attribute("manufacturer"),conflict)||!assign(result.model,child->Attribute("modelName"),conflict))
                return failure(MetadataStatus::Invalid,"Invalid camera name");
        }
        if(local_name(child->Name())!="AcquisitionRecord")continue;
        for(auto* group=child->FirstChildElement();group;group=group->NextSiblingElement()) {
            if(local_name(group->Name())=="Group"&&element_namespace(group)!=ns)return failure(MetadataStatus::Invalid,"Foreign namespace in camera metadata");
            const char* name=group->Attribute("name");
            if(local_name(group->Name())!="Group"||!name||std::string_view(name)!="CameraUnitMetadataSet")continue;
            for(auto* item=group->FirstChildElement();item;item=item->NextSiblingElement()) {
                if(local_name(item->Name())!="Item")continue;
                if(element_namespace(item)!=ns)return failure(MetadataStatus::Invalid,"Foreign namespace in camera metadata");
                const char* key=item->Attribute("name");if(!key)continue;
                std::string* target=nullptr;
                if(std::string_view(key)=="CaptureGammaEquation")target=&result.gamma;
                if(std::string_view(key)=="CaptureColorPrimaries")target=&result.primaries;
                if(std::string_view(key)=="CodingEquations")target=&result.coding;
                if(target&&!assign(*target,item->Attribute("value"),conflict))return failure(MetadataStatus::Invalid,"Invalid camera color declaration");
            }
        }
    }
    if(conflict)return failure(MetadataStatus::Ambiguous,"Conflicting camera metadata");
    if(result.manufacturer.empty()&&result.model.empty()&&result.gamma.empty()&&result.primaries.empty())return result;
    result.status=MetadataStatus::Found;
    result.provenance="Sony NRT";
    const auto gamma=normalized(result.gamma),primaries=normalized(result.primaries);
    if(gamma=="rec709"&&primaries=="rec709")result.profile=DeclaredProfile::Rec709;
    else if(normalized(result.manufacturer)=="sony"&&(gamma=="slog3"||gamma=="sonyslog3")&&
            (primaries=="sgamut3cine"||primaries=="sonysgamut3cine"))result.profile=DeclaredProfile::SonySLog3Cine;
    else if(!gamma.empty()&&!primaries.empty())result.profile=DeclaredProfile::Other;
    result.message="Source declarations; AE or prior effects may already have converted the pixels";
    return result;
}
CameraMetadata parse_xmp_metadata(std::string_view xml,MetadataLimits limits){
    if(xml.size()>limits.xml_bytes)return failure(MetadataStatus::LimitExceeded,"XMP exceeds byte budget");
    if(xml.find('\0')!=std::string_view::npos||xml.find("<!DOCTYPE")!=std::string_view::npos||xml.find("<!ENTITY")!=std::string_view::npos)return failure(MetadataStatus::Invalid,"Unsupported XMP declaration or binary data");
    tinyxml2::XMLDocument document;if(document.Parse(xml.data(),xml.size())!=tinyxml2::XML_SUCCESS)return failure(MetadataStatus::Invalid,"Invalid XMP XML");
    const auto* root=document.RootElement();if(!root)return failure(MetadataStatus::Invalid,"Missing XMP root");if(root->NextSiblingElement())return failure(MetadataStatus::Ambiguous,"Multiple XMP roots");
    constexpr std::string_view rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#",tiff="http://ns.adobe.com/tiff/1.0/";
    if(local_name(root->Name())=="xmpmeta"&&element_namespace(root)=="adobe:ns:meta/"){root=root->FirstChildElement();if(!root||root->NextSiblingElement())return failure(MetadataStatus::Invalid,"Invalid XMP RDF container");}
    if(local_name(root->Name())!="RDF"||element_namespace(root)!=rdf)return failure(MetadataStatus::Unsupported,"Not XMP RDF");
    CameraMetadata result;bool conflict=false;
    for(auto* d=root->FirstChildElement();d;d=d->NextSiblingElement()){
        if(local_name(d->Name())!="Description"||element_namespace(d)!=rdf)continue;
        bool related=false;for(auto* a=d->FirstAttribute();a;a=a->Next())if(local_name(a->Name())=="about"&&qualified_namespace(d,a->Name(),true)==rdf&&*a->Value())related=true;
        if(related)continue;
        auto target=[&](std::string_view name)->std::string*{return name=="Make"?&result.manufacturer:name=="Model"?&result.model:nullptr;};
        for(auto* a=d->FirstAttribute();a;a=a->Next())if(qualified_namespace(d,a->Name(),true)==tiff)if(auto* field=target(local_name(a->Name())))if(!assign(*field,a->Value(),conflict))return failure(MetadataStatus::Invalid,"Invalid XMP camera name");
        for(auto* p=d->FirstChildElement();p;p=p->NextSiblingElement())if(element_namespace(p)==tiff)if(auto* field=target(local_name(p->Name()))){
            if(p->FirstChildElement())return failure(MetadataStatus::Invalid,"Complex XMP camera property");
            for(auto* a=p->FirstAttribute();a;a=a->Next()){const std::string_view name(a->Name());if(name!="xmlns"&&name.find("xmlns:")!=0)return failure(MetadataStatus::Invalid,"Qualified XMP camera property unsupported");}
            std::string text;for(auto* n=p->FirstChild();n;n=n->NextSibling())if(auto* t=n->ToText()){if(text.size()+std::string_view(t->Value()).size()>128)return failure(MetadataStatus::Invalid,"Oversized XMP camera name");text+=t->Value();}
            if(!assign(*field,text.c_str(),conflict))return failure(MetadataStatus::Invalid,"Invalid XMP camera name");
        }
    }
    if(conflict)return failure(MetadataStatus::Ambiguous,"Conflicting XMP camera declarations");
    if(!result.manufacturer.empty()||!result.model.empty()){result.status=MetadataStatus::Found;result.provenance="XMP TIFF";result.message="Camera name only; input color profile is not declared";}
    return result;
}
CameraMetadata read_camera_metadata(const std::filesystem::path& path,MetadataLimits limits) {
    try {
        if(!limits.boxes||limits.read_bytes<16||!limits.xml_bytes)return failure(MetadataStatus::LimitExceeded,"Empty metadata budget");
        const auto file_size=std::filesystem::file_size(path);
        if(file_size<8||file_size>static_cast<std::uintmax_t>(std::numeric_limits<std::streamoff>::max()))return failure(MetadataStatus::Unsupported,"Unsupported media container size");
        std::ifstream stream(path,std::ios::binary);if(!stream)return failure(MetadataStatus::IoError,"Cannot open source metadata");
        std::size_t read=0,boxes=0;
        CameraMetadata found;
        auto finish=[&](CameraMetadata value){value.bytes_read=read;return value;};
        const auto read_at=[&](std::uint64_t at,void* bytes,std::size_t count) {
            if(count>limits.read_bytes-read||at>file_size||count>file_size-at)return false;
            stream.seekg(static_cast<std::streamoff>(at));stream.read(static_cast<char*>(bytes),static_cast<std::streamsize>(count));
            read+=count;return bool(stream);
        };
        std::function<CameraMetadata(std::uint64_t,std::uint64_t,unsigned,std::string_view)> scan;
        scan=[&](std::uint64_t offset,std::uint64_t end,unsigned depth,std::string_view parent)->CameraMetadata {
          if(depth>8)return failure(MetadataStatus::LimitExceeded,"Metadata container depth exceeded");
          while(offset<end) {
            if(++boxes>limits.boxes)return finish(failure(MetadataStatus::LimitExceeded,"Metadata box count exceeded"));
            if(end-offset<8)return failure(MetadataStatus::Invalid,"Truncated media box header");
            if(limits.read_bytes-read<16)return finish(failure(MetadataStatus::LimitExceeded,"Metadata read budget exceeded"));
            std::array<unsigned char,16> header{};
            if(!read_at(offset,header.data(),8))return failure(MetadataStatus::IoError,"Metadata header read failed");
            const std::string_view type(reinterpret_cast<const char*>(header.data()+4),4);
            if(offset==0&&depth==0&&type!="ftyp"&&type!="moov"&&type!="mdat"&&type!="wide"&&type!="free"&&type!="skip")return failure(MetadataStatus::Unsupported,"MP4/MOV camera metadata only");
            auto length=big_endian(header.data(),4);std::uint64_t header_size=8;
            if(length==1){if(end-offset<16)return failure(MetadataStatus::Invalid,"Truncated extended box header");
                if(!read_at(offset+8,header.data()+8,8))return failure(MetadataStatus::IoError,"Extended metadata header read failed");
                length=big_endian(header.data()+8,8);header_size=16;}
            if(length==0)length=end-offset;
            if(length<header_size||length>end-offset)return failure(MetadataStatus::Invalid,"Invalid media box length");
            const auto payload=offset+header_size,box_end=offset+length;
            bool xmp_uuid=false;
            if(type=="uuid"&&depth==0){
                constexpr std::array<unsigned char,16> xmp_id{{0xbe,0x7a,0xcf,0xcb,0x97,0xa9,0x42,0xe8,0x9c,0x71,0x99,0x94,0x91,0xe3,0xaf,0xac}};
                if(box_end-payload<16)return failure(MetadataStatus::Invalid,"Truncated UUID box");std::array<unsigned char,16> id{};
                if(limits.read_bytes-read<16)return failure(MetadataStatus::LimitExceeded,"Metadata read budget exceeded");
                if(!read_at(payload,id.data(),id.size()))return failure(MetadataStatus::IoError,"Metadata UUID read failed");xmp_uuid=id==xmp_id;
            }
            // Parse known container structure. Never promote arbitrary XML substrings.
            if(type=="meta"||type=="moov"||type=="udta") {
                auto children=payload;
                if(type=="meta") {
                    std::array<unsigned char,4> flags{};
                    if(box_end-payload<4)return failure(MetadataStatus::Invalid,"Truncated meta full-box header");
                    if(!read_at(payload,flags.data(),4))return failure(MetadataStatus::LimitExceeded,"Metadata read budget exceeded");
                    if(big_endian(flags.data(),4)!=0)return failure(MetadataStatus::Unsupported,"Unsupported metadata box version");
                    children+=4;
                }
                auto result=scan(children,box_end,depth+1,type);
                if(result.status!=MetadataStatus::NotFound)return result;
            } else if(type=="xml "||xmp_uuid||(type=="XMP_"&&parent=="udta")) {
                const bool sony=type=="xml ";auto start=payload+(xmp_uuid?16:0);
                if(sony){if(box_end-payload<4)return failure(MetadataStatus::Invalid,"Truncated XML full-box header");
                std::array<unsigned char,4> flags{};
                if(!read_at(payload,flags.data(),4))return failure(MetadataStatus::LimitExceeded,"Metadata read budget exceeded");
                if(big_endian(flags.data(),4)!=0)return failure(MetadataStatus::Unsupported,"Unsupported XML box version");
                start+=4;}
                const auto size=box_end-start;
                if(size>limits.xml_bytes||size>limits.read_bytes-read)return failure(MetadataStatus::LimitExceeded,"Metadata XML exceeds byte budget");
                std::string xml(static_cast<std::size_t>(size),'\0');
                if(!read_at(start,xml.data(),xml.size()))return failure(MetadataStatus::IoError,"Camera XML read failed");
                // ISO XML strings may have terminal NUL padding; interior NUL stays invalid.
                if(sony)while(!xml.empty()&&xml.back()=='\0')xml.pop_back();
                auto current=sony?parse_sony_metadata_xml(xml,limits):parse_xmp_metadata(xml,limits);
                if(current.status!=MetadataStatus::Found&&current.status!=MetadataStatus::NotFound)return current;
                if(current.status==MetadataStatus::Found) {
                    if(!merge(found,current))return failure(MetadataStatus::Ambiguous,"Conflicting metadata boxes");
                }
            }
            offset+=length;
          }
          return CameraMetadata{};
        };
        auto scanned=scan(0,file_size,0,{});
        if(scanned.status!=MetadataStatus::NotFound)return finish(scanned);
        return finish(found);
    }catch(const std::filesystem::filesystem_error&){return failure(MetadataStatus::IoError,"Source metadata unavailable");}
     catch(const std::ios_base::failure&){return failure(MetadataStatus::IoError,"Source metadata read failed");}
}
const char* metadata_status_name(MetadataStatus value) noexcept {
    switch(value){case MetadataStatus::Found:return "found";case MetadataStatus::NotFound:return "not_found";case MetadataStatus::Unsupported:return "unsupported";case MetadataStatus::Invalid:return "invalid";case MetadataStatus::Ambiguous:return "ambiguous";case MetadataStatus::LimitExceeded:return "limit_exceeded";case MetadataStatus::IoError:return "io_error";}return "unknown";
}
const char* declared_profile_name(DeclaredProfile value) noexcept {
    switch(value){case DeclaredProfile::Rec709:return "Rec.709 (SDR)";case DeclaredProfile::SonySLog3Cine:return "Sony S-Log3 / S-Gamut3.Cine";case DeclaredProfile::Other:return "Other declared profile";case DeclaredProfile::Unknown:return "Unknown";}return "Unknown";
}
}
