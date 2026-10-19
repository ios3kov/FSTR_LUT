#include "row_iteration.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <chrono>
using namespace fstr::ae;
void check(bool v,const char*s){if(!v)throw std::runtime_error(s);}
using Callback=std::int32_t(*)(void*,std::int32_t,std::int32_t,std::int32_t);
std::int32_t host(std::int32_t rows,void* ref,Callback f){if(rows<=0)return 0;auto first=f(ref,0,0,rows);if(first)return first;std::atomic<int> next{1},error{0};auto work=[&](int t){while(!error){auto y=next.fetch_add(1);if(y>=rows)break;auto e=f(ref,t,y,rows);if(e)error=e;}};std::vector<std::thread> workers;for(int t=1;t<4;++t)workers.emplace_back(work,t);work(0);for(auto&t:workers)t.join();return error;}
int main(){try{
 std::vector<int> out(1000,-1);check(iterate_rows(1000,host,[&](int y,bool){out[y]=y*y;return 0;})==0,"host result");for(int i=0;i<1000;++i)check(out[i]==i*i,"row output");
 bool first=false;try{iterate_rows(1000,host,[&](int y,bool){if(y==2){std::this_thread::sleep_for(std::chrono::milliseconds(5));throw std::runtime_error("early");}if(y==20)throw std::runtime_error("late");return 0;});}catch(const std::runtime_error&e){first=std::string(e.what())=="early";}check(first,"worker completion order replaced earliest error");
 check(iterate_rows(1000,host,[&](int,bool poll){return poll?75:0;})==75,"cancel not propagated");
 auto main=std::this_thread::get_id();std::atomic<bool> bad{false};iterate_rows(1000,host,[&](int,bool poll){if(poll&&std::this_thread::get_id()!=main)bad=true;return 0;});check(!bad,"nonzero SDK thread may poll abort");
 char a[100],b[100];check(disjoint_rows(a,10,10,b,10,10),"separate spans");check(!disjoint_rows(a,10,10,a+10,9,10),"overlap");check(!disjoint_rows(a,1,100,a,1,100),"same buffer");check(disjoint_rows(a,1,50,a+50,1,50),"adjacent spans");check(!disjoint_rows(a,std::size_t(-1),100,b,1,1),"span overflow");check(!disjoint_rows(reinterpret_cast<void*>(std::uintptr_t(-5)),1,10,b,1,1),"address overflow");
 for(int r=0;r<50;++r){std::vector<int> values(100);iterate_rows(100,host,[&](int y,bool){values[y]=r+y;return 0;});for(int y=0;y<100;++y)check(values[y]==r+y,"lifetime/repeated iteration");}
 std::cout<<"PASS host-owned concurrent rows, deterministic errors, cancellation, lifetime and alias overflow fallback\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
