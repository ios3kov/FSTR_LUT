#include "fstr/external_lut.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>
#include <chrono>
#include <future>
#include <cstring>
#include <iostream>
using namespace fstr;
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){auto root=std::filesystem::temp_directory_path()/("fstr-file-lut-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{root};try{
 auto save=[&](const char* name,const std::string& data){auto p=root/name;std::ofstream f(p,std::ios::binary);f<<data;f.close();return p.string();};
 std::ostringstream cube,csp,hex;cube<<"TITLE \"Permutation\"\nLUT_3D_SIZE 2\n";csp<<"CSPLUTV100\n3D\nBEGIN METADATA\nauthored reference\nEND METADATA\n";for(int c=0;c<3;++c)csp<<"2\n0 1\n0 1\n";csp<<"2 2 2\n";
 for(int b=0;b<2;++b)for(int g=0;g<2;++g)for(int r=0;r<2;++r){cube<<g<<' '<<b<<' '<<r<<'\n';csp<<g<<' '<<b<<' '<<r<<'\n';for(float f:{float(g),float(b),float(r)}){std::uint32_t bits;std::memcpy(&bits,&f,4);for(int i=0;i<4;++i){const auto byte=(bits>>(8*i))&255;hex<<"0123456789abcdef"[byte>>4]<<"0123456789abcdef"[byte&15];}}}
 auto a=save("reference.cube",cube.str()),b=save("reference.csp",csp.str()),c=save("reference.look","<look><LUT><size>\"2\"</size><data>\""+hex.str()+"\"</data></LUT></look>");
 for(const auto& path:{a,b,c}){auto lut=load_external_lut(path);check(lut.identity.size()==64&&lut.look,"Missing transform identity");for(auto p:std::vector<RGB>{{0,0,0},{1,0,0},{0,1,0},{0,0,1},{.2,.4,.7}}){auto out=lut.look->sample(p);check(std::abs(out.r-p.g)<3e-6&&std::abs(out.g-p.b)<3e-6&&std::abs(out.b-p.r)<3e-6,"Format/node/channel permutation mismatch");}}
 std::ostringstream three;three<<"3DMESH\nMesh 4 12\n";for(int i=0;i<17;++i)three<<(i==16?1023:i*64)<<' ';three<<'\n';for(int r=0;r<17;++r)for(int g=0;g<17;++g)for(int bl=0;bl<17;++bl)three<<std::lround(4095.*g/16)<<' '<<std::lround(4095.*bl/16)<<' '<<std::lround(4095.*r/16)<<'\n';auto d=save("reference.3dl",three.str());auto dl=load_external_lut(d);auto out=dl.look->sample({1,0,0});check(std::abs(out.r)<1e-5&&std::abs(out.g)<1e-5&&std::abs(out.b-1)<1e-5,"3DL node order or integer scaling mismatch");
 auto memory=load_external_lut(a);std::filesystem::remove(a);check(std::abs(memory.look->sample({.2,.4,.7}).r-.4)<3e-6,"Loaded transform depends on file during sampling");bool missing=false;try{load_external_lut(a);}catch(const std::exception&){missing=true;}check(missing,"Missing external source silently reused");
 a=save("reference.cube",cube.str());auto same=load_external_lut(a);check(same.identity==memory.identity,"Same bytes changed identity");auto modified=cube.str();modified.replace(modified.find("0 0 0\n"),6,"0.1 0 0\n");save("reference.cube",modified);check(load_external_lut(a).identity!=same.identity,"Changed file bytes reused old processor/identity");
 std::vector<std::future<void>> jobs;for(int i=0;i<8;++i)jobs.push_back(std::async(std::launch::async,[&]{for(int k=0;k<1000;++k)check(std::abs(memory.look->sample({.2,.4,.7}).g-.7)<3e-6,"Concurrent sampler changed");}));for(auto& job:jobs)job.get();
 for(auto item:std::vector<std::pair<const char*,std::string>>{{"bad.cube","LUT_3D_SIZE 100000\n"},{"bad.look","<look><LUT><size>100000</size><data>0</data></LUT></look>"},{"bad.csp","CSPLUTV100\n3D\nBEGIN METADATA\nno terminator"},{"bad.3dl","nonsense"},{"bad.txt",cube.str()},{"truncated.cube","LUT_3D_SIZE 2\n0 0 0\n"}}){bool rejected=false;try{load_external_lut(save(item.first,item.second));}catch(const std::exception&){rejected=true;}check(rejected,"Malformed/unsupported LUT accepted");}
 bool directory=false;try{load_external_lut(root.string());}catch(const std::exception&){directory=true;}check(directory,"Directory accepted");std::cout<<"PASS four native LUT formats, independent channel nodes/scaling, exact source identity/reload, memory lifetime, concurrency and invalid input\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
