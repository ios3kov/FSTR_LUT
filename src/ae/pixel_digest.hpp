#pragma once
#include "fstr/byte_sha256.hpp"
#include <cstddef>
#include <type_traits>
namespace fstr::ae {
template<class P>void hash_argb_row(ByteSha256& hash,const void* row,std::size_t width){
    static_assert(std::is_standard_layout_v<P> && offsetof(P,alpha)==0 && offsetof(P,red)==sizeof(P::alpha) && offsetof(P,green)==2*sizeof(P::alpha) && offsetof(P,blue)==3*sizeof(P::alpha) && sizeof(P)==4*sizeof(P::alpha),"Exact packed ARGB required for source identity");
    if(width>std::size_t(-1)/sizeof(P))throw std::invalid_argument("Pixel hash row overflow");
    hash.update(row,width*sizeof(P));
}
}
