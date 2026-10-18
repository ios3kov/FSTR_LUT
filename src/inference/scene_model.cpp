#include "fstr/scene_model.hpp"
#include <cstdlib>
#ifdef FSTR_SCENE_COREML
#include <sys/sysctl.h>
#include "fstr/compiler_cache_generation.hpp"
#endif
#include "onnxruntime_cxx_api.h"
#include "picosha2.h"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <thread>
#include <dlfcn.h>
namespace fstr {namespace {
double smooth(double a,double b,double v){double t=std::clamp((v-a)/(b-a),0.,1.);return t*t*(3-2*t);}
std::string digest(const std::filesystem::path& path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("Scene model asset unavailable");std::string hash;picosha2::hash256_hex_string(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>(),hash);if(f.bad())throw std::runtime_error("Scene model asset read failed");return hash;}
}
struct SceneModel::Impl {
#ifdef FSTR_SCENE_COREML
 std::unique_ptr<CompilerCacheGeneration> compiler_generation; // Outlives session and Env.
#endif
 Ort::Env env{ORT_LOGGING_LEVEL_WARNING,"FSTR_Scene"};Ort::SessionOptions options;std::unique_ptr<Ort::Session> session;std::array<int,150> roles{};std::mutex device;std::string runtime;
 explicit Impl(const std::string& directory,const std::function<void()>& cancel){
  if(cancel)cancel();
  const auto root=std::filesystem::path(directory);if(!root.is_absolute())throw std::invalid_argument("Absolute scene model path required");
  if(std::string(OrtGetApiBase()->GetVersionString())!="1.23.2")throw std::runtime_error("Unsupported native ONNX Runtime version");Dl_info runtime_info{};if(!dladdr(reinterpret_cast<void*>(&OrtGetApiBase),&runtime_info)||!runtime_info.dli_fname)throw std::runtime_error("Native runtime identity unavailable");runtime=digest(runtime_info.dli_fname);
  if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(root/"manifest.json"))||std::filesystem::file_size(root/"manifest.json")>131072)throw std::invalid_argument("Scene manifest budget");
  std::ifstream f(root/"manifest.json");auto manifest=nlohmann::json::parse(f);
  if(manifest.at("schema")!=1||manifest.at("input").at("shape")!=std::vector<int>{1,3,512,512}||manifest.at("output").at("shape")!=std::vector<int>{1,150,512,512})throw std::invalid_argument("Scene model tensor contract mismatch");
  if(manifest.at("source").at("files").at("model.safetensors")!="cd55b1d36c6602d34f8cd300d7f788ea0fa5c3ed928e9ac98f353ecb31c7fa1d"||manifest.at("input").at("mean")!=std::vector<double>{.485,.456,.406}||manifest.at("input").at("std")!=std::vector<double>{.229,.224,.225})throw std::invalid_argument("Scene normalization/provenance mismatch");
  const auto path=root/"scene.onnx";if(!std::filesystem::is_regular_file(std::filesystem::symlink_status(path))||std::filesystem::file_size(path)>512*1024*1024||digest(path)!=manifest.at("onnx_sha256"))throw std::runtime_error("Scene model checksum mismatch");
  const auto&labels=manifest.at("output").at("id2label");if(labels.size()!=150||labels.at("12")!="person")throw std::invalid_argument("Scene model labels mismatch");
  const std::array<std::set<std::string>,5> groups{{{"sky"},{"tree","grass","plant","field","flower","palm"},{"water","sea","river","waterfall","lake","swimming pool"},{"wall","building","floor","ceiling","road","sidewalk","house","skyscraper","bridge","path","stairs","fence"},{"lamp","light","chandelier","streetlight"}}};
  roles.fill(-1);for(int i=0;i<150;++i)for(int r=0;r<5;++r)if(groups[std::size_t(r)].count(labels.at(std::to_string(i)).get<std::string>()))roles[std::size_t(i)]=r+3;
  options.SetIntraOpNumThreads(4);options.SetInterOpNumThreads(1);options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
#ifdef FSTR_SCENE_COREML
  char os_build[128]{};std::size_t os_size=sizeof(os_build);
  if(sysctlbyname("kern.osversion",os_build,&os_size,nullptr,0)||!os_size||os_size>sizeof(os_build)||os_build[os_size-1]!=0)throw std::runtime_error("CoreML OS identity unavailable");
  std::filesystem::path cache_root;
  if(const char* configured=std::getenv("FSTR_LUT_COREML_CACHE"))cache_root=configured;
  else {const char* home=std::getenv("HOME");if(!home)throw std::runtime_error("CoreML cache location unavailable");cache_root=std::filesystem::path(home)/"Library/Caches/FSTR.Tools/FSTR_LUT/coreml-v2";}
  if(!cache_root.is_absolute())throw std::invalid_argument("Absolute CoreML cache path required");
  const auto compiler_cache=cache_root/os_build/(manifest.at("onnx_sha256").get<std::string>()+"-"+runtime+"-"+picosha2::hash256_hex_string(path.string())+"-MLProgram-ALL-static-cache-v2");
  std::filesystem::create_directories(compiler_cache);
  compiler_generation=std::make_unique<CompilerCacheGeneration>(compiler_cache,cancel);
  options.AppendExecutionProvider("CoreML",{{"ModelFormat","MLProgram"},{"MLComputeUnits","ALL"},{"RequireStaticInputShapes","1"},{"ModelCacheDirectory",compiler_generation->directory().string()}});
  // Never enable ProfileComputePlan in the host (ORT1.23.2 can stall on errors).
  runtime+="/CoreML-MLProgram-ALL-static-v1/";runtime+=os_build;
#endif
  if(cancel)cancel();
  session=std::make_unique<Ort::Session>(env,path.c_str(),options);
  if(session->GetInputCount()!=1||session->GetOutputCount()!=1||session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape()!=std::vector<std::int64_t>{1,3,512,512}||session->GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape()!=std::vector<std::int64_t>{1,150,512,512})throw std::invalid_argument("Scene runtime tensor contract mismatch");
#ifdef FSTR_SCENE_COREML
  compiler_generation->commit();
#endif
 }
};
SceneModel::SceneModel(const std::string& p,const std::function<void()>& cancel):impl_(new Impl(p,cancel)){}
SceneModel::~SceneModel()=default;
const std::string& SceneModel::runtime_identity()const noexcept{return impl_->runtime;}
SceneModelResult SceneModel::analyze(const AnalysisImage& image,const PersonParts&parts,const std::function<void()>& cancel){
 const int w=image.width,h=image.height;if(w<1||h<1||w>2048||h>2048||std::size_t(w)*std::size_t(h)>1048576||image.rgba.size()!=std::size_t(w)*std::size_t(h)*4||parts.width!=w||parts.height!=h||parts.probabilities.size()!=std::size_t(w)*std::size_t(h))throw std::invalid_argument("Scene model input contract mismatch");
 auto abort=[&]{if(cancel)cancel();};abort();std::unique_lock<std::mutex> lock(impl_->device,std::defer_lock);while(!lock.try_lock()){abort();std::this_thread::sleep_for(std::chrono::milliseconds(2));}
 const std::size_t plane=512*512;std::vector<float> input(3*plane);const double means[]{.485,.456,.406},stds[]{.229,.224,.225};
 for(int y=0;y<512;++y){abort();double ay=std::clamp((y+.5)*h/512.-.5,0.,double(h-1));int y0=int(ay),y1=std::min(y0+1,h-1);double fy=ay-y0;
  for(int x=0;x<512;++x){double ax=std::clamp((x+.5)*w/512.-.5,0.,double(w-1));int x0=int(ax),x1=std::min(x0+1,w-1);double fx=ax-x0;
   for(int c=0;c<3;++c){auto pixel=[&](int xx,int yy){return image.rgba[(std::size_t(yy)*std::size_t(w)+std::size_t(xx))*4+std::size_t(c)];};double v=(1-fy)*((1-fx)*pixel(x0,y0)+fx*pixel(x1,y0))+fy*((1-fx)*pixel(x0,y1)+fx*pixel(x1,y1));input[std::size_t(c)*plane+std::size_t(y)*512+std::size_t(x)]=float((std::floor(v+.5)/255.-means[c])/stds[c]);}
  }
 }
 const std::int64_t shape[]{1,3,512,512};auto memory=Ort::MemoryInfo::CreateCpu(OrtArenaAllocator,OrtMemTypeDefault);auto tensor=Ort::Value::CreateTensor<float>(memory,input.data(),input.size(),shape,4);const char* inputs[]{"normalized_srgb"};const char* outputs[]{"logits"};auto result=impl_->session->Run(Ort::RunOptions{nullptr},inputs,&tensor,1,outputs,1);abort();
 if(result.size()!=1||!result[0].IsTensor()||result[0].GetTensorTypeAndShapeInfo().GetShape()!=std::vector<std::int64_t>{1,150,512,512})throw std::runtime_error("Scene inference output mismatch");const auto logits=result[0].GetTensorData<float>();
 SceneModelResult out;out.width=w;out.height=h;out.weights.resize(std::size_t(w)*std::size_t(h));out.person.resize(out.weights.size());
 double sums[3]{},squares[3]{};for(std::size_t i=0;i<out.weights.size();++i)for(std::size_t c=0;c<3;++c){double v=image.rgba[i*4+c]/255.;sums[c]+=v;squares[c]+=v*v;}double deviation=0;for(int c=0;c<3;++c){double mean=sums[c]/double(out.weights.size());deviation=std::max(deviation,std::sqrt(std::max(0.,squares[c]/double(out.weights.size())-mean*mean)));}const double information=smooth(.005,.025,deviation);
 for(int y=0;y<h;++y){abort();double ay=std::clamp((y+.5)*512/h-.5,0.,511.);int y0=int(ay),y1=std::min(y0+1,511);double fy=ay-y0;
  for(int x=0;x<w;++x){double ax=std::clamp((x+.5)*512/w-.5,0.,511.);int x0=int(ax),x1=std::min(x0+1,511);double fx=ax-x0;std::array<double,150> values{};double peak=-1e30;
   for(std::size_t c=0;c<150;++c){auto at=[&](int xx,int yy){return logits[c*plane+std::size_t(yy)*512+std::size_t(xx)];};const double v=(1-fy)*((1-fx)*at(x0,y0)+fx*at(x1,y0))+fy*((1-fx)*at(x0,y1)+fx*at(x1,y1));if(!std::isfinite(v))throw std::runtime_error("Nonfinite scene logit");values[c]=v;peak=std::max(peak,v);}
   double total=0,max_prob=0;for(auto&v:values){v=std::exp(v-peak);total+=v;}const std::size_t i=std::size_t(y)*std::size_t(w)+std::size_t(x);auto&roles=out.weights[i];for(std::size_t c=0;c<150;++c){values[c]/=total;max_prob=std::max(max_prob,values[c]);if(impl_->roles[c]>=0)roles[std::size_t(impl_->roles[c])]+=float(values[c]);}
   double part_sum=0;for(float p:parts.probabilities[i]){if(!std::isfinite(p)||p<0||p>1)throw std::invalid_argument("Invalid part probability");part_sum+=p;}if(part_sum>1.001)throw std::invalid_argument("Invalid part probability mass");const auto&part=parts.probabilities[i];roles[0]=float(values[12]*(part[2]+part[3]));roles[1]=float(values[12]*part[1]);roles[2]=float(values[12]*part[4]);out.person[i]=float(values[12]);const double confidence=smooth(.35,.8,max_prob)*information;for(auto&v:roles)v=float(v*confidence);
  }
 }
 // Same source-guided 5x5 confidence smoothing as the independent reference.
 auto refined=out.weights;
 for(int y=0;y<h;++y){abort();for(int x=0;x<w;++x){std::array<double,8> sum{};double den=0;const auto i=std::size_t(y)*std::size_t(w)+std::size_t(x);
  for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx){int xx=std::clamp(x+dx,0,w-1),yy=std::clamp(y+dy,0,h-1);auto j=std::size_t(yy)*std::size_t(w)+std::size_t(xx);double distance=0;for(std::size_t c=0;c<3;++c){double d=(double(image.rgba[i*4+c])-image.rgba[j*4+c])/255.;distance+=d*d/3.;}double k=std::exp(-(dx*dx+dy*dy)/4.-distance/.02);den+=k;for(std::size_t c=0;c<8;++c)sum[c]+=k*out.weights[j][c];}
  double mass=0;for(std::size_t c=0;c<8;++c){refined[i][c]=float(sum[c]/den);mass+=refined[i][c];}if(mass>1)for(float&v:refined[i])v=float(v/mass);validate_scene_weights(refined[i]);
 }}out.weights=std::move(refined);return out;
}
}
