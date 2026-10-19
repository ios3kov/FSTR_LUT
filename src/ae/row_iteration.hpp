#pragma once
#include <cstdint>
#include <cstddef>
#include <exception>
#include <limits>
#include <mutex>
#include <utility>
#include <type_traits>
namespace fstr::ae {
// Conservative bounding spans, including row padding. Overflow/aliasing keeps
// the serial path; this function never dereferences either borrowed pointer.
inline bool disjoint_rows(const void* a,std::size_t height_a,std::size_t stride_a,const void* b,std::size_t height_b,std::size_t stride_b)noexcept{
    const auto limit=std::numeric_limits<std::uintptr_t>::max();
    if(!a||!b||!height_a||!height_b||!stride_a||!stride_b||height_a>limit/stride_a||height_b>limit/stride_b)return false;
    const auto aa=reinterpret_cast<std::uintptr_t>(a),bb=reinterpret_cast<std::uintptr_t>(b),na=height_a*stride_a,nb=height_b*stride_b;
    if(aa>limit-na||bb>limit-nb)return false;
    return aa+na<=bb||bb+nb<=aa;
}
// Synchronous host-owned iteration. No C++ exception crosses the C callback.
// Host errors/cancellation take precedence. Otherwise preserve the earliest
// failed row, rather than whichever worker happened to finish first.
template<class Iterator,class Process>std::int32_t iterate_rows(std::int32_t rows,Iterator iterator,Process&& process){
    using Function=std::remove_reference_t<Process>;
    struct Context {
        Function& process;std::mutex mutex;std::exception_ptr error;std::int32_t first=std::numeric_limits<std::int32_t>::max();
        static std::int32_t callback(void* raw,std::int32_t thread,std::int32_t row,std::int32_t){
            auto& c=*static_cast<Context*>(raw);
            try{return c.process(row,thread==0);}
            catch(...){std::lock_guard<std::mutex> lock(c.mutex);if(row<c.first){c.first=row;c.error=std::current_exception();}return 0;}
        }
    } context{process,{},nullptr};
    const auto result=iterator(rows,&context,&Context::callback);
    if(result)return result;
    if(context.error)std::rethrow_exception(context.error);
    return 0;
}
}
