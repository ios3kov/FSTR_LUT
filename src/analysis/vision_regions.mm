#include "fstr/regions.hpp"
#import <Vision/Vision.h>
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <stdexcept>
namespace fstr {
std::vector<ColorRegion> detect_color_regions(const AnalysisImage& image, double source_height){
 if(!std::isfinite(source_height)||source_height<=0||image.width<1||image.height<1||image.width>2048||image.height>2048||image.rgba.size()!=std::size_t(image.width)*image.height*4)throw std::invalid_argument("Invalid analysis image");
 static std::mutex device;std::lock_guard<std::mutex> guard(device);
 @autoreleasepool {
 @try {
  CGColorSpaceRef space=CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
  if(!space)throw std::runtime_error("Analysis color space failed");
  CGDataProviderRef provider=CGDataProviderCreateWithData(nullptr,image.rgba.data(),image.rgba.size(),nullptr);
  CGImageRef cg=provider?CGImageCreate(image.width,image.height,8,32,image.width*4,space,kCGImageAlphaNoneSkipLast|kCGBitmapByteOrderDefault,provider,nullptr,false,kCGRenderingIntentDefault):nullptr;
  if(provider)CGDataProviderRelease(provider);CGColorSpaceRelease(space);
  if(!cg)throw std::runtime_error("Analysis image failed");
  VNImageRequestHandler* handler=[[VNImageRequestHandler alloc] initWithCGImage:cg options:@{}];CGImageRelease(cg);
  VNDetectFaceRectanglesRequest* request=[[VNDetectFaceRectanglesRequest alloc] init];request.revision=VNDetectFaceRectanglesRequestRevision3;
  request.usesCPUOnly=YES;
  NSError* error=nil;
  if(![handler performRequests:@[request] error:&error])throw std::runtime_error("Native face analysis failed");
  std::vector<ColorRegion> regions;
  for(VNFaceObservation* face in request.results){
   if(face.confidence<.5)continue;CGRect b=face.boundingBox;
   ColorRegion r;r.x0=std::clamp(double(b.origin.x),0.,1.);r.x1=std::clamp(double(b.origin.x+b.size.width),0.,1.);r.y0=std::clamp(double(1-b.origin.y-b.size.height),0.,1.);r.y1=std::clamp(double(1-b.origin.y),0.,1.);
   if(r.x1<=r.x0||r.y1<=r.y0)continue;
   // Local fine-detail energy; weak evidence can only reduce facial holes.
   double energy=0;int count=0;
   const int x0=std::max(1,int(r.x0*image.width)),x1=std::min(image.width-1,int(r.x1*image.width));
   const int y0=std::max(1,int(r.y0*image.height)),y1=std::min(image.height-1,int(r.y1*image.height));
   auto lum=[&](int x,int y){auto p=&image.rgba[(std::size_t(y)*image.width+x)*4];return (.2126*p[0]+.7152*p[1]+.0722*p[2])/255.;};
   for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){double v=lum(x,y)-.25*(lum(x-1,y)+lum(x+1,y)+lum(x,y-1)+lum(x,y+1));energy+=v*v;++count;}
   double t=std::clamp((std::sqrt(energy/std::max(1,count))-.005)/.025,0.,1.);r.detail=t*t*(3-2*t);
   VNFaceLandmarks2D* landmarks=nil;
   if((r.y1-r.y0)*source_height>=64 && r.detail>0){
    VNDetectFaceLandmarksRequest* detailed=[[VNDetectFaceLandmarksRequest alloc] init];
    detailed.revision=VNDetectFaceLandmarksRequestRevision3;detailed.usesCPUOnly=YES;
    detailed.inputFaceObservations=@[face];
    if(![handler performRequests:@[detailed] error:&error])throw std::runtime_error("Native face detail failed");
    landmarks=detailed.results.firstObject.landmarks;
   }
   for(id item in @[landmarks.leftEye?:[NSNull null],landmarks.rightEye?:[NSNull null],landmarks.innerLips?:[NSNull null]]){
    VNFaceLandmarkRegion2D* part=(VNFaceLandmarkRegion2D*)item;
    if((id)part==[NSNull null])continue;
    std::vector<std::array<double,2>> polygon;
    for(NSUInteger i=0;i<part.pointCount;++i){auto p=part.normalizedPoints[i];polygon.push_back({b.origin.x+p.x*b.size.width,1-(b.origin.y+p.y*b.size.height)});}
    if(polygon.size()>=3)r.holes.push_back(std::move(polygon));
   }
   regions.push_back(std::move(r));if(regions.size()==16)break;
  }
  std::sort(regions.begin(),regions.end(),[](const auto&a,const auto&b){return a.x0<b.x0||(a.x0==b.x0&&a.y0<b.y0);});return regions;
 } @catch(NSException* exception){throw std::runtime_error("Native analysis exception");}
 }
}
}
