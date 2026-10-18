#include "external_lut_ui.hpp"
#import <Cocoa/Cocoa.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <pthread.h>
#include <stdexcept>
namespace fstr::ae {
std::string choose_lut_file(const std::string& current){
    if(!pthread_main_np())throw std::runtime_error("Import LUT requires the main thread");
    @autoreleasepool {
        NSOpenPanel* panel=[NSOpenPanel openPanel];panel.canChooseDirectories=NO;panel.canChooseFiles=YES;panel.allowsMultipleSelection=NO;
        NSMutableArray<UTType*>* types=[NSMutableArray array];for(NSString* ext in @[@"cube",@"3dl",@"look",@"csp"]){UTType* type=[UTType typeWithFilenameExtension:ext conformingToType:UTTypeData];if(type)[types addObject:type];}panel.allowedContentTypes=types;panel.title=@"Import LUT";panel.prompt=@"Import";
        if(!current.empty()){NSString* path=[NSString stringWithUTF8String:current.c_str()];if(path)panel.directoryURL=[NSURL fileURLWithPath:[path stringByDeletingLastPathComponent] isDirectory:YES];}
        if([panel runModal]!=NSModalResponseOK)return {};
        const char* name=[[panel.URL path] UTF8String];if(!name)throw std::runtime_error("Invalid LUT filename");return name;
    }
}
}
