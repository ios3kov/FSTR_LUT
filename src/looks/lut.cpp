#include "fstr/core.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
namespace fstr {
namespace {
bool finite(RGB v) { return std::isfinite(v.r)&&std::isfinite(v.g)&&std::isfinite(v.b); }
std::array<double,3> channels(RGB v) { return {v.r,v.g,v.b}; }
}
LUT LUT::parse(std::istream& stream) {
    LUT lut; std::string line; std::size_t bytes=0, line_number=0;
    bool seen_min=false,seen_max=false,seen_title=false;
    auto next_line=[&](){
        line.clear();char ch=0;
        while(stream.get(ch)) {
            if(++bytes>32u*1024u*1024u) throw std::invalid_argument("LUT exceeds parser byte budget");
            if(ch=='\n') return true;
            if(line.size()>=4096) throw std::invalid_argument("LUT exceeds parser line budget");
            line.push_back(ch);
        }
        return !line.empty();
    };
    while (next_line()) {
        ++line_number;
        bool quoted=false,escaped=false;
        for(std::size_t i=0;i<line.size();++i) {
            char ch=line[i];
            if(ch=='#' && !quoted) {line.resize(i);break;}
            if(ch=='"' && !escaped) quoted=!quoted;
            escaped=ch=='\\' && !escaped;
        }
        std::istringstream row(line); row.imbue(std::locale::classic());
        std::string tag; if(!(row>>tag)) continue;
        auto bad=[&](){ throw std::invalid_argument("Invalid .cube line "+std::to_string(line_number)); };
        auto end=[&](){ std::string extra; if(row>>extra) bad(); };
        if(tag=="TITLE") {
            std::string title; if(seen_title || !(row>>std::quoted(title))) bad();
            seen_title=true; end();
        } else if(tag=="LUT_3D_SIZE") {
            if(lut.size_ || !lut.nodes_.empty() || !(row>>lut.size_) || lut.size_<2 || lut.size_>65) bad();
            end(); lut.nodes_.reserve(lut.size_*lut.size_*lut.size_);
        } else if(tag=="DOMAIN_MIN" || tag=="DOMAIN_MAX") {
            bool& seen=tag=="DOMAIN_MIN"?seen_min:seen_max;
            RGB v{}; if(seen || !lut.nodes_.empty() || !(row>>v.r>>v.g>>v.b) || !finite(v)) bad();
            end(); seen=true;
            (tag=="DOMAIN_MIN"?lut.minimum_:lut.maximum_)=v;
        } else {
            if(!lut.size_ || lut.nodes_.size()>=lut.size_*lut.size_*lut.size_) bad();
            std::istringstream values(line); values.imbue(std::locale::classic());
            RGB v{}; std::string extra;
            if(!(values>>v.r>>v.g>>v.b) || (values>>extra) || !finite(v)) bad();
            lut.nodes_.push_back(v);
        }
    }
    if(stream.bad() || !lut.size_ || lut.nodes_.size()!=lut.size_*lut.size_*lut.size_)
        throw std::invalid_argument("Truncated or missing 3D LUT");
    auto lo=channels(lut.minimum_),hi=channels(lut.maximum_);
    for(std::size_t c=0;c<3;++c)
        if(!(hi[c]>lo[c]) || !std::isfinite(hi[c]-lo[c]))
            throw std::invalid_argument("Invalid LUT domain");
    return lut;
}
LUT LUT::identity(std::size_t size) {
    if(size<2 || size>65) throw std::invalid_argument("Unsupported LUT size");
    LUT lut; lut.size_=size;
    for(std::size_t b=0;b<size;++b)
        for(std::size_t g=0;g<size;++g)
            for(std::size_t r=0;r<size;++r)
                lut.nodes_.push_back({double(r)/double(size-1),double(g)/double(size-1),double(b)/double(size-1)});
    return lut;
}
RGB LUT::sample(RGB input) const {
    if(!size_ || !finite(input)) throw std::invalid_argument("Invalid LUT/sample");
    auto in=channels(input),lo=channels(minimum_),hi=channels(maximum_);
    std::array<std::size_t,3> base{}; std::array<double,3> frac{},residual{};
    for(std::size_t c=0;c<3;++c) {
        auto bounded=std::clamp(in[c],lo[c],hi[c]);
        residual[c]=in[c]-bounded;
        auto coordinate=(bounded-lo[c])/(hi[c]-lo[c])*double(size_-1);
        base[c]=std::min(static_cast<std::size_t>(coordinate),size_-2);
        frac[c]=coordinate-double(base[c]);
    }
    std::array<std::size_t,3> order{0,1,2};
    std::stable_sort(order.begin(),order.end(),[&](auto a,auto b){return frac[a]>frac[b];});
    auto node=[&](const auto& p){return channels(nodes_[p[0]+size_*(p[1]+size_*p[2])]);};
    auto a=node(base); auto p=base; ++p[order[0]]; auto b=node(p);
    ++p[order[1]]; auto c=node(p); ++p[order[2]]; auto d=node(p);
    std::array<double,3> out{};
    for(std::size_t ch=0;ch<3;++ch)
        out[ch]=a[ch]+frac[order[0]]*(b[ch]-a[ch])+frac[order[1]]*(c[ch]-b[ch])+frac[order[2]]*(d[ch]-c[ch])+residual[ch];
    RGB result{out[0],out[1],out[2]};
    if(!finite(result)) throw std::invalid_argument("LUT arithmetic exceeded finite range");
    return result;
}
} // namespace fstr
