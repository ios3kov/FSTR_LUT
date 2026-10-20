#pragma once
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
#else
#include "picosha2.h"
#endif
namespace fstr {
// Exact byte-stream SHA256. Batching changes neither byte order nor digest.
// Instances belong to one request; no shared mutable platform context.
class ByteSha256 final {
#ifdef __APPLE__
    CC_SHA256_CTX state_{};
#else
    picosha2::hash256_one_by_one state_;
#endif
    bool finished_=false;
public:
    ByteSha256(){
#ifdef __APPLE__
        if(CC_SHA256_Init(&state_)!=1)throw std::runtime_error("SHA256 initialization failed");
#else
        state_.init();
#endif
    }
    void update(const void* data,std::size_t count){
        if(finished_)throw std::logic_error("SHA256 already finished");
        if(count&&!data)throw std::invalid_argument("Missing SHA256 bytes");
        const auto* p=static_cast<const unsigned char*>(data);
        while(count){const auto n=std::min<std::size_t>(count,65536);
#ifdef __APPLE__
            if(CC_SHA256_Update(&state_,p,CC_LONG(n))!=1)throw std::runtime_error("SHA256 update failed");
#else
            state_.process(p,p+n);
#endif
            p+=n;count-=n;
        }
    }
    std::string finish(){
        if(finished_)throw std::logic_error("SHA256 already finished");
        finished_=true;
#ifdef __APPLE__
        unsigned char digest[CC_SHA256_DIGEST_LENGTH];if(CC_SHA256_Final(digest,&state_)!=1)throw std::runtime_error("SHA256 finish failed");
        constexpr char hex[]="0123456789abcdef";std::string result;result.reserve(64);for(auto v:digest){result.push_back(hex[v>>4]);result.push_back(hex[v&15]);}return result;
#else
        state_.finish();return picosha2::get_hash_hex_string(state_);
#endif
    }
};
inline std::string byte_sha256(const std::string& bytes){ByteSha256 hash;hash.update(bytes.data(),bytes.size());return hash.finish();}
}
