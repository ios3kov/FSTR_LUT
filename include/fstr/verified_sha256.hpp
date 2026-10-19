#pragma once
#include <array>
#include <istream>
#include <stdexcept>
#include <string>
#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#else
#include "picosha2.h"
#endif
namespace fstr {
// Hash all file bytes each time. No identity/mtime shortcut and no large copy.
inline std::string verified_sha256(std::istream& input) {
#ifdef __APPLE__
    CC_SHA256_CTX state;if(CC_SHA256_Init(&state)!=1)throw std::runtime_error("SHA256 initialization failed");
    std::array<char,64*1024> buffer{};
    while(input){input.read(buffer.data(),std::streamsize(buffer.size()));const auto n=input.gcount();
        if(n>0&&CC_SHA256_Update(&state,buffer.data(),CC_LONG(n))!=1)throw std::runtime_error("SHA256 update failed");
    }
    if(input.bad()||(input.fail()&&!input.eof()))throw std::runtime_error("Asset checksum read failed");
    std::array<unsigned char,CC_SHA256_DIGEST_LENGTH> digest{};if(CC_SHA256_Final(digest.data(),&state)!=1)throw std::runtime_error("SHA256 finish failed");
    constexpr char hex[]="0123456789abcdef";std::string result;result.reserve(64);for(auto byte:digest){result.push_back(hex[byte>>4]);result.push_back(hex[byte&15]);}return result;
#else
    std::string result;picosha2::hash256_hex_string(std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>(),result);
    if(input.bad()||(input.fail()&&!input.eof()))throw std::runtime_error("Asset checksum read failed");return result;
#endif
}
}
