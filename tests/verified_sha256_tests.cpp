#include "fstr/verified_sha256.hpp"
#include "picosha2.h"
#include <sstream>
#include <iostream>
#include <random>
void check(bool x){if(!x)throw std::runtime_error("SHA256 contract failed");}
std::string digest(const std::string& bytes){std::istringstream stream(bytes);return fstr::verified_sha256(stream);}
int main(){try{
 check(digest("")=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");check(digest("abc")=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");check(digest(std::string(1000000,'a'))=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
 std::mt19937 random(0x452);
 for(int count:{0,1,55,56,63,64,65,255,65535,65536,65537,131072,1048577}){std::string b(std::size_t(count),'\0');for(auto&c:b)c=char(random()&255);check(digest(b)==picosha2::hash256_hex_string(b));}
 for(auto state:{std::ios::badbit,std::ios::failbit}){std::istringstream stream("abc");stream.setstate(state);bool rejected=false;try{fstr::verified_sha256(stream);}catch(const std::exception&){rejected=true;}check(rejected);}
 struct Broken:std::streambuf{int_type underflow()override{throw std::runtime_error("Injected read failure");}} broken;std::istream stream(&broken);bool rejected=false;try{fstr::verified_sha256(stream);}catch(const std::exception&){rejected=true;}check(rejected);
 std::cout<<"PASS NIST known vectors,13 frozen-picosha binary/chunk cases and read errors\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
