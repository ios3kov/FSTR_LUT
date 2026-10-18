#pragma once
#include "fstr/compiler_cache_lock.hpp"
#include <algorithm>
#include <fstream>
#include <memory>
#include <vector>
namespace fstr {
// INIT serializes constructors/pointers. Shared generation leases survive for
// the entire model lifetime; cleanup never removes artifacts of a live model.
class CompilerCacheGeneration final {
    struct FD{int value=-1;~FD(){if(value>=0)::close(value);}};
public:
    CompilerCacheGeneration(const std::filesystem::path& root,const std::function<void()>& cancel={})
        :writer_(std::make_unique<CompilerCacheLock>(root/".initialization.lock",cancel)),root_(root) {
        const auto selected=read_small(root_/"current-v1",128);
        if(valid_name(selected)){
            const auto candidate=root_/selected;
            if(std::filesystem::is_directory(std::filesystem::symlink_status(candidate))&&read_small(candidate/".complete-v1",64)==stamp()){
                data_=candidate;reused_=true;
            }
        }
        if(data_.empty()){
            const auto prefix="gen-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(::getpid());
            for(int attempt=0;attempt<32;++attempt){auto p=root_/(prefix+"-"+std::to_string(attempt));if(std::filesystem::create_directory(p)){data_=std::move(p);break;}}
            if(data_.empty())throw std::runtime_error("Compiler generation creation failed");
        }
        lease_.value=open_owned(data_/".lease");
        if(::flock(lease_.value,LOCK_SH|LOCK_NB))throw std::runtime_error("Compiler generation lease unavailable");
        prune();
    }
    ~CompilerCacheGeneration(){
        // A failed warm load makes only the pointer suspect; existing sessions
        // keep their immutable directory and lease. Never throw during unwind.
        if(writer_&&reused_)try{if(read_small(root_/"current-v1",128)==data_.filename().string())std::filesystem::rename(root_/"current-v1",root_/"failed-current-v1");}catch(...){}
    }
    const std::filesystem::path& directory()const noexcept{return data_;}
    bool reused()const noexcept{return reused_;}
    void commit(){
        publish(data_/".complete-v1",stamp());publish(root_/"current-v1",data_.filename().string());
        writer_.reset(); // Keep shared lease while inference can use this model.
    }
private:
    static bool valid_name(const std::string& s){
        if(s.size()<9||s.size()>=120||s.rfind("gen-",0)!=0)return false;
        unsigned separators=0;bool digits=false;
        for(auto i=s.begin()+4;i!=s.end();++i){if(*i>='0'&&*i<='9'){digits=true;continue;}if(*i!='-'||!digits||separators==2)return false;++separators;digits=false;}
        return separators==2&&digits;
    }
    static std::string stamp(){return "FSTR_COREML_COMPILER_COMPLETE_V2";}
    static std::string read_small(const std::filesystem::path&p,std::uintmax_t limit){
        const auto state=std::filesystem::symlink_status(p);if(!std::filesystem::is_regular_file(state)||std::filesystem::file_size(p)>limit)return {};
        std::ifstream input(p,std::ios::binary);std::string value(std::istreambuf_iterator<char>(input),{});return input.bad()?std::string{}:value;
    }
    static int open_owned(const std::filesystem::path&p){
        const int fd=::open(p.c_str(),O_CREAT|O_RDWR|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK,0600);if(fd<0)throw std::runtime_error("Compiler metadata unavailable");
        struct stat file{};if(::fstat(fd,&file)||!S_ISREG(file.st_mode)||file.st_uid!=::geteuid()||file.st_nlink!=1||(file.st_mode&077)){::close(fd);throw std::runtime_error("Invalid compiler metadata file");}return fd;
    }
    static void publish(const std::filesystem::path&p,const std::string&value){
        auto temp=p;temp+=".tmp";FD fd;fd.value=open_owned(temp);if(::ftruncate(fd.value,0))throw std::runtime_error("Compiler marker truncation failed");
        std::size_t offset=0;while(offset<value.size()){const auto n=::write(fd.value,value.data()+offset,value.size()-offset);if(n<0&&errno==EINTR)continue;if(n<=0)throw std::runtime_error("Compiler marker write failed");offset+=std::size_t(n);}
        if(::fsync(fd.value))throw std::runtime_error("Compiler marker sync failed");std::filesystem::rename(temp,p);
    }
    void prune(){
        std::vector<std::filesystem::path> old;
        for(const auto&e:std::filesystem::directory_iterator(root_))if(e.path()!=data_&&valid_name(e.path().filename().string())&&std::filesystem::is_directory(e.symlink_status()))old.push_back(e.path());
        std::sort(old.begin(),old.end(),std::greater<std::filesystem::path>());std::size_t inactive=0;
        for(const auto&p:old){
            FD guard;guard.value=open_owned(p/".lease");
            if(::flock(guard.value,LOCK_EX|LOCK_NB)){if(errno==EWOULDBLOCK||errno==EAGAIN||errno==EINTR)continue;throw std::runtime_error("Compiler cleanup lease failed");}
            if(++inactive<=2)continue;
            std::filesystem::remove_all(p); // Own generation; INIT + exclusive lease held.
        }
    }
    // Reverse destruction releases the reader before the writer only on failed
    // construction; normal commit already released INIT before any inference.
    std::unique_ptr<CompilerCacheLock> writer_;std::filesystem::path root_,data_;FD lease_;bool reused_=false;
};
}
