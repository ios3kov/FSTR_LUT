#include "fstr/native_scene_identity.hpp"
#include <chrono>
#include <iostream>
int main(int argc,char**argv){try{if(argc!=2)throw std::runtime_error("Expected resource directory");const auto start=std::chrono::steady_clock::now();const auto policy=fstr::verified_native_scene_policy(argv[1]);std::cout<<policy<<" "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<" ms; no inference sessions\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
