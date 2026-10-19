#include "scene_payload_codec.hpp"
#include <cstring>
#include <iostream>
#include <random>
#include <vector>
using namespace fstr::cache_codec;
void check(bool value){if(!value)throw std::runtime_error("Lossless codec assertion");}
template<class F>void rejects(F fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}check(caught);}
int main(){try{
 std::mt19937 random(0x1843);std::size_t compared=0;
 for(auto count:{0,1,2,3,4,255,256,65535,65536,65537,262144})for(int mode=0;mode<3;++mode){std::string bytes(std::size_t(count),'\0');for(auto&c:bytes)c=mode==0?0:mode==1?char(random()&255):char(compared++&255);auto packed=encode(bytes);check(decode(packed,bytes.size())==bytes);++compared;
  for(std::size_t cut:{std::size_t(0),packed.size()/2,packed.size()-1})rejects([&]{decode(std::string_view(packed).substr(0,cut),bytes.size());});
  rejects([&]{decode(packed+"x",bytes.size());});rejects([&]{decode(packed,bytes.size()+1);});if(bytes.size())rejects([&]{decode(packed,bytes.size()-1);});
  auto bad=packed;bad[bad.size()-1]^=1;rejects([&]{decode(bad,bytes.size());});
 }
 rejects([&]{decode("invalid",byte_budget+1);});rejects([&]{decode(std::string(100,'x'),0);});
 // Arbitrary IEEE bit patterns, including signed zero/nonfinite: codec must be
 // bit-exact; higher-level scene validation remains responsible for semantics.
 const unsigned bits[]{0,0x80000000,0x3f800000,0x7fc00001,0x7f800000,0xff800000};std::string b(reinterpret_cast<const char*>(bits),sizeof(bits));check(decode(encode(b),b.size())==b);
 std::cout<<"PASS "<<compared<<" lossless payload cases; binary/chunk boundaries and malformed/truncated/trailing/overlong/budget streams\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
