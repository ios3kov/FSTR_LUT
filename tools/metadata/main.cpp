#include "fstr/metadata.hpp"
#include "json.hpp"
#include <iostream>
int main(int argc,char** argv) {
    try {
        if(argc!=2){std::cerr<<"Usage: fstr_metadata_inspect source.mp4\n";return 2;}
        const auto value=fstr::read_camera_metadata(argv[1]);
        nlohmann::json output={{"status",fstr::metadata_status_name(value.status)},
            {"declared_profile",fstr::declared_profile_name(value.profile)},
            {"manufacturer",value.manufacturer},{"camera",value.model},{"gamma",value.gamma},
            {"primaries",value.primaries},{"coding",value.coding},{"provenance",value.provenance},{"bytes_read",value.bytes_read},
            {"scope","Source declarations only; AE pixel encoding and prior conversions remain explicit"}};
        std::cout<<output.dump(2)<<'\n';
        return value.status==fstr::MetadataStatus::Found||value.status==fstr::MetadataStatus::NotFound?0:1;
    }catch(const std::exception& e){std::cerr<<"Metadata inspection failed: "<<e.what()<<'\n';return 2;}
}
