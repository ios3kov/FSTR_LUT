#pragma once
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <zlib.h>
namespace fstr::cache_codec {
constexpr std::size_t byte_budget=64*1024*1024;
inline std::string encode(std::string_view bytes) {
    if(bytes.size()>byte_budget)throw std::invalid_argument("Scene compression budget");
    uLongf length=compressBound(uLong(bytes.size()));std::string out(std::size_t(length),'\0');
    const unsigned char empty=0;const auto* data=bytes.empty()?&empty:reinterpret_cast<const Bytef*>(bytes.data());
    if(compress2(reinterpret_cast<Bytef*>(out.data()),&length,data,uLong(bytes.size()),Z_BEST_SPEED)!=Z_OK)throw std::runtime_error("Scene compression failed");
    out.resize(std::size_t(length));return out;
}
inline std::string decode(std::string_view bytes,std::size_t expected) {
    if(expected>byte_budget||bytes.size()>std::size_t(compressBound(uLong(expected)))||bytes.empty())throw std::invalid_argument("Scene decompression budget");
    std::string out(std::max<std::size_t>(expected,1),'\0');z_stream stream{};
    stream.next_in=reinterpret_cast<Bytef*>(const_cast<char*>(bytes.data()));stream.avail_in=uInt(bytes.size());
    stream.next_out=reinterpret_cast<Bytef*>(out.data());stream.avail_out=uInt(out.size());
    if(inflateInit(&stream)!=Z_OK)throw std::runtime_error("Scene decompression initialization failed");
    struct End{z_stream* stream;~End(){inflateEnd(stream);}} end{&stream};
    const auto code=inflate(&stream,Z_FINISH);
    if(code!=Z_STREAM_END||stream.total_out!=expected||stream.avail_in!=0)throw std::runtime_error("Scene decompression size/stream mismatch");
    out.resize(expected);return out;
}
}
