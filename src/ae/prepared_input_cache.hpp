#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace fstr::ae {
// Host state equality, cancellation and preparation run outside the mutex.
// Values own their pixels; neither host worlds nor effect_ref are retained.
// Concurrent misses may duplicate work, avoiding waits on other host workers.
template<class State, class Value> class PreparedInputCache final {
    struct Entry {State state;std::string key;std::shared_ptr<const Value> value;std::size_t bytes;};
    using Item=std::shared_ptr<const Entry>;
    std::mutex mutex_;
    std::vector<Item> entries_; // least recently used first
    std::size_t bytes_=0,limit_,count_;
    std::uint64_t generation_=0;
public:
    explicit PreparedInputCache(std::size_t bytes=32*1024*1024,std::size_t count=64):limit_(bytes),count_(count){}
    template<class Same,class Build,class Cancel>
    std::shared_ptr<const Value> get(const State& state,const std::string& key,Same same,Build build,Cancel cancel,bool* hit=nullptr){
        if(hit)*hit=false;
        cancel();std::vector<Item> snapshot;std::uint64_t generation;
        {std::lock_guard<std::mutex> lock(mutex_);snapshot=entries_;generation=generation_;}
        for(const auto& e:snapshot)if(e->key==key&&same(e->state,state)){
            cancel();
            {std::lock_guard<std::mutex> lock(mutex_);auto i=std::find(entries_.begin(),entries_.end(),e);if(i!=entries_.end())std::rotate(i,i+1,entries_.end());}
            if(hit)*hit=true;return e->value;
        }
        auto value=std::make_shared<const Value>(build());cancel();
        // Include retained metadata and control blocks in the conservative budget.
        const auto payload=value->cache_bytes();
        if(!count_||key.size()>limit_||payload>limit_-key.size()||sizeof(Entry)+128>limit_-key.size()-payload)return value;
        const auto bytes=payload+key.size()+sizeof(Entry)+128;
        auto fresh=std::make_shared<const Entry>(Entry{state,key,value,bytes});
        std::vector<Item> retired;
        {std::lock_guard<std::mutex> lock(mutex_);
            if(generation!=generation_)return value;
            while(!entries_.empty()&&(entries_.size()>=count_||bytes_>limit_-bytes)){retired.push_back(std::move(entries_.front()));bytes_-=retired.back()->bytes;entries_.erase(entries_.begin());}
            entries_.push_back(std::move(fresh));bytes_+=bytes;
        }
        return value;
    }
    void clear(){std::vector<Item> retired;{std::lock_guard<std::mutex> lock(mutex_);retired.swap(entries_);bytes_=0;++generation_;}}
};
}
