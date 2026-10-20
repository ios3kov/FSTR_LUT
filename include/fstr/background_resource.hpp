#pragma once
#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace fstr {
// One owned initialization worker. Factories must own all their inputs and must
// not call host APIs. Request cancellation never abandons the loading worker.
template<class T> class BackgroundResource final {
public:
    enum class State { Idle, Loading, Ready, Failed, Stopped };
    using Factory=std::function<std::shared_ptr<T>()>;
    BackgroundResource()=default;
    BackgroundResource(const BackgroundResource&)=delete;
    BackgroundResource& operator=(const BackgroundResource&)=delete;
    ~BackgroundResource(){stop();}

    void start(Factory factory) {
        std::lock_guard<std::mutex> lifecycle(lifecycle_);
        std::lock_guard<std::mutex> lock(mutex_);
        if(state_!=State::Idle&&state_!=State::Stopped)return;
        state_=State::Loading;error_=nullptr;
        try {
            worker_=std::thread([this,factory=std::move(factory)] {
                std::shared_ptr<T> ready;std::exception_ptr error;
                try {ready=factory();if(!ready)throw std::runtime_error("Empty background resource");}
                catch(...){error=std::current_exception();}
                {
                    std::lock_guard<std::mutex> guard(mutex_);
                    if(state_==State::Loading){value_=std::move(ready);error_=error;state_=error?State::Failed:State::Ready;}
                }
                changed_.notify_all();
                // A result completed during stop is destroyed here before join.
            });
        }catch(...){error_=std::current_exception();state_=State::Failed;changed_.notify_all();}
    }
    State state()const {std::lock_guard<std::mutex> lock(mutex_);return state_;}
    std::shared_ptr<T> ready()const {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_==State::Ready?value_:std::shared_ptr<T>{};
    }
    std::shared_ptr<T> wait(const std::function<void()>& cancel={})const {
        for(;;){
            if(cancel)cancel(); // Host callback belongs only to the calling thread.
            std::unique_lock<std::mutex> lock(mutex_);
            if(state_==State::Ready)return value_;
            if(state_==State::Failed)std::rethrow_exception(error_);
            if(state_!=State::Loading)throw std::runtime_error("Background resource is stopped");
            changed_.wait_for(lock,std::chrono::milliseconds(5),[&]{return state_!=State::Loading;});
        }
    }
    void stop() {
        std::lock_guard<std::mutex> lifecycle(lifecycle_);
        std::shared_ptr<T> retired;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            state_=State::Stopped;retired=std::move(value_);error_=nullptr;
        }
        changed_.notify_all();
        if(worker_.joinable())worker_.join();
        // Outstanding requests retain their own shared ownership. Host teardown
        // must finish those callbacks before unloading this library's code.
    }
private:
    mutable std::mutex mutex_;
    std::mutex lifecycle_;
    mutable std::condition_variable changed_;
    State state_=State::Idle;
    std::shared_ptr<T> value_;
    std::exception_ptr error_;
    std::thread worker_;
};
}
