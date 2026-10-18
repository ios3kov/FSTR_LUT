#pragma once
// CoreML compiler cache is shared by processes; its initialization writer must
// be exclusive even when two independent AE hosts start at the same time.
#include <chrono>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <thread>
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
namespace fstr {
class CompilerCacheLock final {
public:
    explicit CompilerCacheLock(const std::filesystem::path& file,
        const std::function<void()>& cancel={},
        std::chrono::milliseconds timeout=std::chrono::minutes(5)) {
        if(!file.is_absolute()||timeout.count()<0)throw std::invalid_argument("Invalid compiler lock configuration");
        fd_=::open(file.c_str(),O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
        if(fd_<0)throw std::runtime_error("Could not open compiler cache lock");
        try {
            struct stat opened{};
            if(::fstat(fd_,&opened)||!S_ISREG(opened.st_mode)||opened.st_uid!=::geteuid()||opened.st_nlink!=1||(opened.st_mode&077))throw std::runtime_error("Unsafe compiler cache lock file");
            const auto deadline=std::chrono::steady_clock::now()+timeout;
            for(;;){
                if(cancel)cancel();
                if(!::flock(fd_,LOCK_EX|LOCK_NB))break;
                if(errno!=EWOULDBLOCK&&errno!=EAGAIN&&errno!=EINTR)throw std::runtime_error("Could not lock compiler cache");
                if(std::chrono::steady_clock::now()>=deadline)throw std::runtime_error("Compiler cache lock timed out");
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            struct stat named{};
            if(::lstat(file.c_str(),&named)||named.st_dev!=opened.st_dev||named.st_ino!=opened.st_ino)throw std::runtime_error("Compiler cache lock was replaced");
        }catch(...){::close(fd_);fd_=-1;throw;}
    }
    ~CompilerCacheLock(){if(fd_>=0)::close(fd_);}
    CompilerCacheLock(const CompilerCacheLock&)=delete;
    CompilerCacheLock& operator=(const CompilerCacheLock&)=delete;
private:
    int fd_=-1;
};
}
