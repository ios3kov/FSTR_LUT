#include "fstr/verified_sha256.hpp"
#include "fstr/parts_model.hpp"
#include "mediapipe_abi.hpp"
#include "picosha2.h"
#include <algorithm>
#include <cmath>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <thread>
namespace fstr {namespace {
constexpr const char* model_sha="c6748b1253a99067ef71f7e26ca71096cd449baefa8f101900ea23016507e0e0";
void verify(const std::string& path,const std::string& expected){
 if(!std::filesystem::path(path).is_absolute()||expected.size()!=64||!std::filesystem::is_regular_file(std::filesystem::symlink_status(path))||std::filesystem::file_size(path)>512*1024*1024)throw std::invalid_argument("Invalid native model asset");
 std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("Native model asset unavailable");std::string actual;actual=verified_sha256(input);if(input.bad()||actual!=expected)throw std::runtime_error("Native model asset checksum mismatch");
}
void check_image(const AnalysisImage&i){if(i.width<1||i.height<1||i.width>2048||i.height>2048||i.rgba.size()!=std::size_t(i.width)*std::size_t(i.height)*4)throw std::invalid_argument("Invalid native model input");}
}
struct PartsModel::Impl {
 void* library=nullptr;void* segmenter=nullptr;void* face=nullptr;void* pose=nullptr;std::mutex device;
 using ErrorFree=void(*)(char*);ErrorFree error_free=nullptr;
 using Create=int(*)(const mpabi::Options*,void**,char**);Create create=nullptr;
 using Segment=int(*)(void*,void*,const void*,mpabi::Result*,char**);Segment segment=nullptr;
 using Close=int(*)(void*,char**);Close close=nullptr;
 using CloseResult=void(*)(mpabi::Result*);CloseResult close_result=nullptr;
 using ImageCreate=int(*)(int,int,int,const std::uint8_t*,int,void**,char**);ImageCreate image_create=nullptr;
 using ImageFree=void(*)(void*);ImageFree image_free=nullptr;
 using ImageInt=int(*)(void*);ImageInt width=nullptr,height=nullptr,step=nullptr,format=nullptr,channels=nullptr;
 using ImageData=int(*)(void*,const float**,char**);ImageData data=nullptr;
 using Labels=int(*)(void*,mpabi::Strings*,char**);Labels labels=nullptr;
 using FreeStrings=void(*)(void*);FreeStrings free_strings=nullptr;
 template<class T>T symbol(const char* name){auto p=dlsym(library,name);if(!p)throw std::runtime_error("Incompatible native model runtime");return reinterpret_cast<T>(p);}
 void checked(int status,char* error){std::string message;try{if(status)message=error?error:"Native model inference failed";}catch(...){if(error)error_free(error);throw;}if(error)error_free(error);if(status)throw std::runtime_error(message);}
 template<class F>void call(F f){char* error=nullptr;int status=f(&error);checked(status,error);}
 using FaceCreate=int(*)(const mpabi::FaceOptions*,void**,char**);FaceCreate face_create=nullptr;
 using FaceDetect=int(*)(void*,void*,const void*,mpabi::FaceResult*,char**);FaceDetect face_detect=nullptr;
 using FaceFree=void(*)(mpabi::FaceResult*);FaceFree face_free=nullptr;Close face_close=nullptr;
 using PoseCreate=int(*)(const mpabi::PoseOptions*,void**,char**);PoseCreate pose_create=nullptr;
 using PoseDetect=int(*)(void*,void*,const void*,mpabi::PoseResult*,char**);PoseDetect pose_detect=nullptr;
 using PoseFree=void(*)(mpabi::PoseResult*);PoseFree pose_free=nullptr;Close pose_close=nullptr;
 Impl(const std::string& path,const std::string& hash,const std::string& model,const std::string& face_path,const std::string& pose_path){
  verify(path,hash);verify(model,model_sha);
  library=dlopen(path.c_str(),RTLD_NOW|RTLD_LOCAL);if(!library)throw std::runtime_error("Native model runtime load failed");
  try{
   error_free=symbol<ErrorFree>("MpErrorFree");create=symbol<Create>("MpImageSegmenterCreate");segment=symbol<Segment>("MpImageSegmenterSegmentImage");close=symbol<Close>("MpImageSegmenterClose");close_result=symbol<CloseResult>("MpImageSegmenterCloseResult");image_create=symbol<ImageCreate>("MpImageCreateFromUint8Data");image_free=symbol<ImageFree>("MpImageFree");width=symbol<ImageInt>("MpImageGetWidth");height=symbol<ImageInt>("MpImageGetHeight");step=symbol<ImageInt>("MpImageGetWidthStep");format=symbol<ImageInt>("MpImageGetFormat");channels=symbol<ImageInt>("MpImageGetChannels");data=symbol<ImageData>("MpImageDataFloat32");labels=symbol<Labels>("MpImageSegmenterGetLabels");free_strings=symbol<FreeStrings>("MpStringListFree");
   mpabi::Options options;options.base_options.model_asset_path=model.c_str();call([&](char** e){return create(&options,&segmenter,e);});if(!segmenter)throw std::runtime_error("Native segmenter missing");
   mpabi::Strings names;struct NamesGuard{Impl* owner;mpabi::Strings* p;~NamesGuard(){owner->free_strings(p);}} guard{this,&names};
   call([&](char** e){return labels(segmenter,&names,e);});const char* expected[]{"background","hair","body-skin","face-skin","clothes","others"};
   if(names.count!=6||!names.strings)throw std::runtime_error("Native model label count mismatch");for(int c=0;c<6;++c)if(!names.strings[c]||std::string(names.strings[c])!=expected[c])throw std::runtime_error("Native model label order mismatch");
   if(!face_path.empty()){
    verify(face_path,"64184e229b263107bc2b804c6625db1341ff2bb731874b0bcc2fe6544e0bc9ff");face_create=symbol<FaceCreate>("MpFaceLandmarkerCreate");face_detect=symbol<FaceDetect>("MpFaceLandmarkerDetectImage");face_free=symbol<FaceFree>("MpFaceLandmarkerCloseResult");face_close=symbol<Close>("MpFaceLandmarkerClose");mpabi::FaceOptions o;o.base_options.model_asset_path=face_path.c_str();call([&](char**e){return face_create(&o,&face,e);});if(!face)throw std::runtime_error("Missing face model");
   }
   if(!pose_path.empty()){
    verify(pose_path,"4eaa5eb7a98365221087693fcc286334cf0858e2eb6e15b506aa4a7ecdcec4ad");pose_create=symbol<PoseCreate>("MpPoseLandmarkerCreate");pose_detect=symbol<PoseDetect>("MpPoseLandmarkerDetectImage");pose_free=symbol<PoseFree>("MpPoseLandmarkerCloseResult");pose_close=symbol<Close>("MpPoseLandmarkerClose");mpabi::PoseOptions o;o.base_options.model_asset_path=pose_path.c_str();call([&](char**e){return pose_create(&o,&pose,e);});if(!pose)throw std::runtime_error("Missing person model");
   }
  }catch(...){shutdown();throw;}
 }
 void shutdown()noexcept{for(auto pair:{std::make_pair(&pose,pose_close),std::make_pair(&face,face_close)})if(*pair.first&&pair.second){char* error=nullptr;pair.second(*pair.first,&error);if(error&&error_free)error_free(error);*pair.first=nullptr;}if(segmenter&&close){char* error=nullptr;close(segmenter,&error);if(error&&error_free)error_free(error);segmenter=nullptr;}if(library){dlclose(library);library=nullptr;}}
 ~Impl(){shutdown();}
};
PartsModel::PartsModel(const std::string& l,const std::string& s,const std::string& m,const std::string& f,const std::string& p):impl_(new Impl(l,s,m,f,p)){}
PartsModel::~PartsModel()=default;
PersonParts PartsModel::analyze(const AnalysisImage& image,const std::function<void()>& cancel){
 check_image(image);auto abort=[&]{if(cancel)cancel();};abort();
 std::unique_lock<std::mutex> lock(impl_->device,std::defer_lock);while(!lock.try_lock()){abort();std::this_thread::sleep_for(std::chrono::milliseconds(2));}abort();
 void* input=nullptr;struct InputGuard{Impl* owner;void** p;~InputGuard(){if(*p)owner->image_free(*p);}} input_guard{impl_.get(),&input};
 impl_->call([&](char** e){return impl_->image_create(2,image.width,image.height,image.rgba.data(),static_cast<int>(image.rgba.size()),&input,e);});if(!input)throw std::runtime_error("Native model input missing");
 mpabi::Result result;struct ResultGuard{Impl* owner;mpabi::Result* p;~ResultGuard(){owner->close_result(p);}} result_guard{impl_.get(),&result};
 impl_->call([&](char**e){return impl_->segment(impl_->segmenter,input,nullptr,&result,e);});abort();
 if(result.confidence_masks_count!=6||!result.confidence_masks)throw std::runtime_error("Native model channel count mismatch");
 PersonParts output;output.width=image.width;output.height=image.height;output.probabilities.resize(std::size_t(image.width)*std::size_t(image.height));
 for(int c=0;c<6;++c){auto mask=result.confidence_masks[c];if(!mask||impl_->width(mask)!=image.width||impl_->height(mask)!=image.height||impl_->format(mask)!=9||impl_->channels(mask)!=1)throw std::runtime_error("Native mask contract mismatch");
  const int stride=impl_->step(mask);if(stride<image.width*4||stride%4)throw std::runtime_error("Native mask stride mismatch");const float* data=nullptr;impl_->call([&](char**e){return impl_->data(mask,&data,e);});if(!data)throw std::runtime_error("Native mask data missing");
  for(int y=0;y<image.height;++y){abort();for(int x=0;x<image.width;++x){const float value=data[std::size_t(y)*std::size_t(stride/4)+std::size_t(x)];if(!std::isfinite(value)||value<0||value>1)throw std::runtime_error("Invalid native model probability");output.probabilities[std::size_t(y)*std::size_t(image.width)+std::size_t(x)][std::size_t(c)]=value;}}
 }
 for(const auto&p:output.probabilities){double sum=0;for(float v:p)sum+=v;if(sum>1.001)throw std::runtime_error("Native model probability mass mismatch");}return output;
}
std::vector<std::array<float,3>> PartsModel::face_mesh(const AnalysisImage&image,const std::function<void()>&cancel){
 check_image(image);if(!impl_->face)throw std::runtime_error("Face model unavailable");auto abort=[&]{if(cancel)cancel();};abort();std::unique_lock<std::mutex> lock(impl_->device,std::defer_lock);while(!lock.try_lock()){abort();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 void* input=nullptr;struct InputGuard{Impl*o;void**p;~InputGuard(){if(*p)o->image_free(*p);}} guard{impl_.get(),&input};impl_->call([&](char**e){return impl_->image_create(2,image.width,image.height,image.rgba.data(),int(image.rgba.size()),&input,e);});
 mpabi::FaceResult result;struct ResultGuard{Impl*o;mpabi::FaceResult*p;~ResultGuard(){o->face_free(p);}} result_guard{impl_.get(),&result};impl_->call([&](char**e){return impl_->face_detect(impl_->face,input,nullptr,&result,e);});abort();if(result.count>1)throw std::runtime_error("Face result count mismatch");if(!result.count)return {};if(!result.landmarks||!result.landmarks[0].landmarks||result.landmarks[0].count!=478)throw std::runtime_error("Face mesh shape mismatch");std::vector<std::array<float,3>> out;out.reserve(478);for(std::size_t i=0;i<478;++i){auto&p=result.landmarks[0].landmarks[i];if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))throw std::runtime_error("Nonfinite face landmark");out.push_back({p.x,p.y,p.z});}return out;
}
std::vector<std::vector<float>> PartsModel::people(const AnalysisImage&image,const std::function<void()>&cancel){
 check_image(image);if(!impl_->pose)throw std::runtime_error("Person model unavailable");auto abort=[&]{if(cancel)cancel();};abort();std::unique_lock<std::mutex> lock(impl_->device,std::defer_lock);while(!lock.try_lock()){abort();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 void* input=nullptr;struct InputGuard{Impl*o;void**p;~InputGuard(){if(*p)o->image_free(*p);}} guard{impl_.get(),&input};impl_->call([&](char**e){return impl_->image_create(2,image.width,image.height,image.rgba.data(),int(image.rgba.size()),&input,e);});
 mpabi::PoseResult result;struct ResultGuard{Impl*o;mpabi::PoseResult*p;~ResultGuard(){o->pose_free(p);}} result_guard{impl_.get(),&result};impl_->call([&](char**e){return impl_->pose_detect(impl_->pose,input,nullptr,&result,e);});abort();if(result.count>4||result.count!=result.landmarks_count||(result.count&&!result.masks))throw std::runtime_error("Person result count mismatch");std::vector<std::vector<float>> out;
 for(std::size_t i=0;i<result.count;++i){void* mask=result.masks[i];if(!mask||impl_->width(mask)!=image.width||impl_->height(mask)!=image.height||impl_->format(mask)!=9||impl_->channels(mask)!=1)throw std::runtime_error("Person mask contract mismatch");int stride=impl_->step(mask);if(stride<image.width*4||stride%4)throw std::runtime_error("Person mask stride mismatch");const float* data=nullptr;impl_->call([&](char**e){return impl_->data(mask,&data,e);});if(!data)throw std::runtime_error("Person mask missing");std::vector<float> field(std::size_t(image.width)*std::size_t(image.height));for(int y=0;y<image.height;++y){abort();for(int x=0;x<image.width;++x){float v=data[std::size_t(y)*std::size_t(stride/4)+std::size_t(x)];if(!std::isfinite(v)||v<0||v>1)throw std::runtime_error("Invalid person confidence");field[std::size_t(y)*std::size_t(image.width)+std::size_t(x)]=v;}}out.push_back(std::move(field));}return out;
}

}
