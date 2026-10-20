#pragma once
#include "fstr/background_resource.hpp"
namespace fstr {
// One initialization worker, created only on the first actual miss. The factory
// receives shutdown cancellation, never a caller's host-thread callback.
template<class T> class DeferredResource final {
public:
    using Factory=std::function<std::shared_ptr<T>(const std::function<void()>&)>;
    explicit DeferredResource(Factory factory):factory_(std::move(factory)) {
        if(!factory_)throw std::invalid_argument("Missing deferred resource factory");
    }
    ~DeferredResource(){loader_.stop();}
    std::shared_ptr<T> get(const std::function<void()>& cancel={}) {
        if(cancel)cancel();
        loader_.start([this]{return factory_([this]{
            if(loader_.state()==BackgroundResource<T>::State::Stopped)
                throw std::runtime_error("Deferred resource stopped");
        });});
        return loader_.wait(cancel);
    }
    typename BackgroundResource<T>::State state()const{return loader_.state();}
private:
    Factory factory_;
    BackgroundResource<T> loader_;
};
}
