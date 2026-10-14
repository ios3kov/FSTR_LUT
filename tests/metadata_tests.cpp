#include "fstr/metadata.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace fstr;
namespace {
int checks=0;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
std::string xml(const std::string& gamma="rec709",const std::string& primaries="rec709") {
    return "<?xml version=\"1.0\"?><NonRealTimeMeta xmlns=\"urn:schemas-professionalDisc:nonRealTimeMeta:ver.2.20\">"
        "<Device manufacturer=\"Sony\" modelName=\"ILME-FX3\" serialNo=\"PRIVATE-NOT-EXPORTED\"/>"
        "<AcquisitionRecord><Group name=\"CameraUnitMetadataSet\"><Item name=\"CaptureGammaEquation\" value=\""+gamma+
        "\"/><Item name=\"CaptureColorPrimaries\" value=\""+primaries+"\"/><Item name=\"CodingEquations\" value=\"rec709\"/></Group>"
        "<Group name=\"ExifGPS\"><Item name=\"GPSLatitude\" value=\"PRIVATE-NOT-EXPORTED\"/></Group></AcquisitionRecord></NonRealTimeMeta>";
}
std::string box(const char* type,const std::string& payload) {
    const auto n=static_cast<unsigned>(payload.size()+8);std::string result;
    for(int shift=24;shift>=0;shift-=8)result.push_back(static_cast<char>((n>>shift)&255));
    result.append(type,4);return result+payload;
}
std::string metadata_box(const std::string& text) {return box("meta",std::string(4,'\0')+box("xml ",std::string(4,'\0')+text));}
struct File {
    std::filesystem::path path;
    File():path(std::filesystem::temp_directory_path()/("fstr-metadata-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".mp4")){}
    ~File(){std::error_code error;std::filesystem::remove(path,error);}
    void write(const std::string& bytes){std::ofstream file(path,std::ios::binary);file.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));if(!file)throw std::runtime_error("Fixture write failed");}
};
}
int main(){try {
    const auto sdr=parse_sony_metadata_xml(xml());
    check(sdr.status==MetadataStatus::Found&&sdr.profile==DeclaredProfile::Rec709,"SDR declarations not recognized");
    check(sdr.manufacturer=="Sony"&&sdr.model=="ILME-FX3"&&sdr.gamma=="rec709"&&sdr.primaries=="rec709","Whitelisted camera fields wrong");
    const auto log=parse_sony_metadata_xml(xml("s-log3","s-gamut3-cine"));
    check(log.profile==DeclaredProfile::SonySLog3Cine,"Explicit Sony curve/gamut not recognized");
    check(parse_sony_metadata_xml(xml("s-log3","s-gamut3")).profile==DeclaredProfile::Other,"Different gamut treated as Cine");
    check(parse_sony_metadata_xml(xml("hlg","rec2020")).profile==DeclaredProfile::Other,"HLG guessed as SDR/Log");
    check(parse_sony_metadata_xml(xml("","rec709")).profile==DeclaredProfile::Unknown,"Missing curve guessed");
    auto no_device=xml();auto a=no_device.find("<Device"),b=no_device.find("/>",a);no_device.erase(a,b+2-a);
    check(parse_sony_metadata_xml(no_device).status==MetadataStatus::Found,"Color declarations require unnecessary camera name");
    auto conflict=xml();a=conflict.find("</Group>");conflict.insert(a,"<Item name=\"CaptureGammaEquation\" value=\"s-log3\"/>");
    check(parse_sony_metadata_xml(conflict).status==MetadataStatus::Ambiguous,"Conflicting fields accepted");
    auto comment=xml();a=comment.find("<AcquisitionRecord>");comment.insert(a,"<!--<Device manufacturer=\"Fake\" modelName=\"Wrong\"/>-->");
    check(parse_sony_metadata_xml(comment).model=="ILME-FX3","Comment promoted to metadata");
    check(parse_sony_metadata_xml("<!DOCTYPE x [<!ENTITY remote SYSTEM 'file:///private'>]>"+xml()).status==MetadataStatus::Invalid,"DTD not rejected");
    check(parse_sony_metadata_xml(xml()+"<Second/>").status!=MetadataStatus::Found,"Multiple roots accepted");
    check(parse_sony_metadata_xml(xml().substr(0,200)).status==MetadataStatus::Invalid,"Truncated XML accepted");
    check(parse_sony_metadata_xml(xml("rec709",std::string(129,'x'))).status==MetadataStatus::Invalid,"Oversized field accepted");
    check(parse_sony_metadata_xml(xml(),MetadataLimits{4096,16,100}).status==MetadataStatus::LimitExceeded,"XML budget ignored");
    auto wrong_namespace=xml();a=wrong_namespace.find("urn:");b=wrong_namespace.find('"',a);wrong_namespace.replace(a,b-a,"unrelated");
    check(parse_sony_metadata_xml(wrong_namespace).status==MetadataStatus::Unsupported,"Namespace ignored");
    for(const auto* tag:{"Device","AcquisitionRecord","Group","Item"}) {
        auto foreign=xml();const auto pos=foreign.find(std::string("<")+tag)+1+std::string(tag).size();
        foreign.insert(pos," xmlns=\"urn:foreign\"");
        check(parse_sony_metadata_xml(foreign).status==MetadataStatus::Invalid,"Foreign child namespace accepted");
    }
    auto prefixed=xml();
    for(const auto* tag:{"NonRealTimeMeta","Device","AcquisitionRecord","Group","Item"}) {
        for(const auto& pattern:{std::string("<")+tag,std::string("</")+tag}) {
            std::size_t pos=0;while((pos=prefixed.find(pattern,pos))!=std::string::npos){prefixed.insert(pos+(pattern[1]=='/'?2:1),"n:");pos+=pattern.size()+2;}
        }
    }
    prefixed.replace(prefixed.find("xmlns="),6,"xmlns:n=");
    check(parse_sony_metadata_xml(prefixed).profile==DeclaredProfile::Rec709,"Valid prefixed namespace rejected");
    File f;const auto ftyp=box("ftyp","XAVC0000");
    f.write(ftyp+box("mdat",std::string(100000,'V'))+metadata_box(xml()));
    auto media=read_camera_metadata(f.path);check(media.status==MetadataStatus::Found&&media.model=="ILME-FX3","Embedded MP4 metadata not recognized");
    check(media.bytes_read<2000,"Compressed frame payload was read");
    f.write(ftyp+metadata_box(xml())+metadata_box(xml("s-log3","s-gamut3-cine")));
    check(read_camera_metadata(f.path).status==MetadataStatus::Ambiguous,"Conflicting boxes accepted");
    f.write(ftyp+metadata_box(xml())+metadata_box(xml()));
    check(read_camera_metadata(f.path).status==MetadataStatus::Found,"Identical metadata boxes rejected");
    f.write(ftyp+box("mdat",std::string(100000,'V')));media=read_camera_metadata(f.path);
    check(media.status==MetadataStatus::NotFound&&media.bytes_read==16,"Missing metadata fabricated");
    f.write(ftyp+std::string("\xff\xff\xff\xffmeta",8));
    check(read_camera_metadata(f.path).status==MetadataStatus::Invalid,"Oversized box accepted");
    f.write(ftyp+std::string("\0\0\0\1meta",8));
    check(read_camera_metadata(f.path).status==MetadataStatus::Invalid,"Extended header truncation ignored");
    f.write(ftyp+metadata_box(xml()));
    check(read_camera_metadata(f.path,MetadataLimits{32,262144,4096}).status!=MetadataStatus::Found,"Read budget ignored");
    check(read_camera_metadata(f.path,MetadataLimits{4194304,262144,1}).status==MetadataStatus::LimitExceeded,"Box budget ignored");
    f.write(ftyp+metadata_box(xml()+xml("s-log3","s-gamut3-cine")));
    check(read_camera_metadata(f.path).status!=MetadataStatus::Found,"Multiple documents within one XML box accepted");
    auto undeclared=xml();undeclared.erase(0,undeclared.find("?>")+2);
    f.write(ftyp+metadata_box("<Wrapper>"+undeclared+"</Wrapper>"));
    check(read_camera_metadata(f.path).status==MetadataStatus::Unsupported,"Nested root promoted from wrapper");
    f.write(ftyp+box("moov",xml()));
    check(read_camera_metadata(f.path).status!=MetadataStatus::Found,"Arbitrary XML in moov promoted to document");
    f.write(ftyp+box("moov",box("udta",metadata_box(prefixed))));
    check(read_camera_metadata(f.path).profile==DeclaredProfile::Rec709,"Structured nested prefixed metadata not recognized");
    f.write(ftyp+metadata_box(xml()+std::string(1,'\0')));
    check(read_camera_metadata(f.path).status==MetadataStatus::Found,"Terminal XML string NUL rejected");
    auto deep=metadata_box(xml());for(int i=0;i<10;++i)deep=box("udta",deep);
    f.write(ftyp+deep);check(read_camera_metadata(f.path).status==MetadataStatus::LimitExceeded,"Container depth budget ignored");
    f.write("not-mp4-source");check(read_camera_metadata(f.path).status==MetadataStatus::Unsupported,"Unknown file scanned as MP4");
    check(read_camera_metadata(f.path.string()+"-missing").status==MetadataStatus::IoError,"Missing source silently succeeded");
    std::cout<<"PASS "<<checks<<" camera metadata contract assertions\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<" assertion "<<checks<<'\n';return 1;}}
