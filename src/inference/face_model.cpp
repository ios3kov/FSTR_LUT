#include "fstr/face_model.hpp"
#include "onnxruntime_cxx_api.h"
#include "picosha2.h"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <thread>
namespace fstr {namespace {
double iou(const std::array<double,4>&a,const std::array<double,4>&b){double s=std::max(0.,std::min(a[2],b[2])-std::max(a[0],b[0]))*std::max(0.,std::min(a[3],b[3])-std::max(a[1],b[1]));return s/std::max(1e-12,(a[2]-a[0])*(a[3]-a[1])+(b[2]-b[0])*(b[3]-b[1])-s);}
}
struct FaceModel::Impl {Ort::Env env{ORT_LOGGING_LEVEL_WARNING,"FSTR_Face"};Ort::SessionOptions options;std::unique_ptr<Ort::Session> session;std::mutex device;
 explicit Impl(const std::string& directory){auto root=std::filesystem::path(directory);if(!root.is_absolute())throw std::invalid_argument("Absolute detector path required");if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(root/"manifest.json"))||std::filesystem::file_size(root/"manifest.json")>16384)throw std::invalid_argument("Detector manifest budget");std::ifstream mf(root/"manifest.json");auto j=nlohmann::json::parse(mf);if(j.at("schema")!=1||j.at("source_sha256")!="ebafce4e3c118d6554634be5c27ab333b4c047a9a8c3faf1d7cf93101c22f0f0")throw std::invalid_argument("Detector provenance mismatch");auto path=root/"yunet.onnx";if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(path))||std::filesystem::file_size(path)>4*1024*1024)throw std::invalid_argument("Detector asset invalid");std::ifstream f(path,std::ios::binary);std::string hash;picosha2::hash256_hex_string(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>(),hash);if(f.bad()||hash!=j.at("sha256"))throw std::runtime_error("Detector checksum mismatch");options.SetIntraOpNumThreads(2);options.SetInterOpNumThreads(1);session=std::make_unique<Ort::Session>(env,path.c_str(),options);if(session->GetInputCount()!=1||session->GetOutputCount()!=12)throw std::invalid_argument("Detector tensor count mismatch");}
};
FaceModel::FaceModel(const std::string&p):impl_(new Impl(p)){}
FaceModel::~FaceModel()=default;
std::vector<FaceProposal> FaceModel::analyze(const AnalysisImage&image,const std::function<void()>&cancel){
 const int w=image.width,h=image.height;if(w<1||h<1||w>2048||h>2048||image.rgba.size()!=std::size_t(w)*std::size_t(h)*4)throw std::invalid_argument("Invalid face detector image");auto abort=[&]{if(cancel)cancel();};abort();std::unique_lock<std::mutex> lock(impl_->device,std::defer_lock);while(!lock.try_lock()){abort();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 std::vector<FaceProposal> proposals;std::vector<std::vector<FaceProposal>> scales;
 for(double scale:{std::min(1.,1536./std::max(w,h)),std::min(2.,1536./std::max(w,h))}){
  const int sw=std::max(1,int(std::nearbyint(w*scale))),sh=std::max(1,int(std::nearbyint(h*scale))),pw=(sw+31)/32*32,ph=(sh+31)/32*32;const auto plane=std::size_t(pw)*std::size_t(ph);std::vector<float> pixels(plane*3);
  for(int y=0;y<sh;++y){abort();double ay=std::clamp((y+.5)*h/sh-.5,0.,double(h-1));int y0=int(ay),y1=std::min(y0+1,h-1);double fy=ay-y0;
   for(int x=0;x<sw;++x){double ax=std::clamp((x+.5)*w/sw-.5,0.,double(w-1));int x0=int(ax),x1=std::min(x0+1,w-1);double fx=ax-x0;for(int c=0;c<3;++c){auto at=[&](int xx,int yy){return image.rgba[(std::size_t(yy)*std::size_t(w)+std::size_t(xx))*4+std::size_t(2-c)];};double v=(1-fy)*((1-fx)*at(x0,y0)+fx*at(x1,y0))+fy*((1-fx)*at(x0,y1)+fx*at(x1,y1));pixels[std::size_t(c)*plane+std::size_t(y)*std::size_t(pw)+std::size_t(x)]=float(std::floor(v+.5));}}
  }
  const std::int64_t shape[]{1,3,ph,pw};auto tensor=Ort::Value::CreateTensor<float>(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault),pixels.data(),pixels.size(),shape,4);const char* inputs[]{"input"};const char* names[]{"cls_8","cls_16","cls_32","obj_8","obj_16","obj_32","bbox_8","bbox_16","bbox_32","kps_8","kps_16","kps_32"};auto values=impl_->session->Run(Ort::RunOptions{nullptr},inputs,&tensor,1,names,12);abort();std::vector<FaceProposal> found;
  for(std::size_t s=0;s<3;++s){const int stride=8<<s,cols=pw/stride,rows=ph/stride;const auto count=std::size_t(rows)*std::size_t(cols);for(std::size_t group=0;group<4;++group){auto&v=values[group*3+s];if(!v.IsTensor()||v.GetTensorTypeAndShapeInfo().GetElementType()!=ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT||v.GetTensorTypeAndShapeInfo().GetElementCount()!=count*(group==2?4:group==3?10:1))throw std::runtime_error("Detector output shape mismatch");}
   const auto cls=values[s].GetTensorData<float>(),obj=values[s+3].GetTensorData<float>(),boxes=values[s+6].GetTensorData<float>();
   for(int y=0;y<rows;++y)for(int x=0;x<cols;++x){auto i=std::size_t(y)*std::size_t(cols)+std::size_t(x);if(!std::isfinite(cls[i])||!std::isfinite(obj[i]))throw std::runtime_error("Nonfinite detector score");double score=std::sqrt(std::clamp(double(cls[i]),0.,1.)*std::clamp(double(obj[i]),0.,1.));if(score<.8)continue;for(std::size_t c=0;c<4;++c)if(!std::isfinite(boxes[i*4+c]))throw std::runtime_error("Nonfinite face box");double cx=(x+boxes[i*4])*stride,cy=(y+boxes[i*4+1])*stride,bw=std::exp(double(boxes[i*4+2]))*stride,bh=std::exp(double(boxes[i*4+3]))*stride;if(!std::isfinite(bw)||!std::isfinite(bh))throw std::runtime_error("Face extent overflow");FaceProposal p{{(cx-bw*.5)*w/sw,(cy-bh*.5)*h/sh,(cx+bw*.5)*w/sw,(cy+bh*.5)*h/sh},score,0};if(std::max({std::abs(cx),std::abs(cy),bw,bh})>1e7)throw std::runtime_error("Invalid detector extent");found.push_back(p);}
  }
  std::stable_sort(found.begin(),found.end(),[](const auto&a,const auto&b){return a.score>b.score;});if(found.size()>5000)found.resize(5000);std::vector<FaceProposal> suppressed,selected;auto integer_box=[&](const auto&b){double x=int(b[0]*sw/w),y=int(b[1]*sh/h);return std::array<double,4>{x,y,x+int((b[2]-b[0])*sw/w),y+int((b[3]-b[1])*sh/h)};};for(const auto&p:found){bool overlap=false;for(const auto&q:suppressed)if(iou(integer_box(p.box),integer_box(q.box))>.3){overlap=true;break;}if(!overlap)suppressed.push_back(p);}for(const auto&p:suppressed){auto b=p.box;double bw=b[2]-b[0],bh=b[3]-b[1];if(b[0]<0||b[1]<0||b[2]>w||b[3]>h||std::min(bw,bh)<1||bw/bh<=.3||bw/bh>=2)continue;selected.push_back(p);if(selected.size()==16)break;}proposals.insert(proposals.end(),selected.begin(),selected.end());scales.push_back(std::move(selected));
 }
 std::sort(proposals.begin(),proposals.end(),[](const auto&a,const auto&b){return a.score>b.score||(a.score==b.score&&a.box<b.box);});std::vector<FaceProposal> selected;for(auto p:proposals){bool overlap=false;for(const auto&q:selected)if(iou(p.box,q.box)>=.35){overlap=true;break;}if(overlap)continue;for(const auto&s:scales)if(std::any_of(s.begin(),s.end(),[&](const auto&q){return iou(p.box,q.box)>.5;}))++p.scale_agreement;selected.push_back(p);if(selected.size()==16)break;}std::sort(selected.begin(),selected.end(),[](const auto&a,const auto&b){return a.box<b.box;});return selected;
}
}
