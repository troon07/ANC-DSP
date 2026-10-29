#include "anc/adaptive.h"
#include "anc/dsp_engine.h"
#include <cstdlib>
#include <new>
#include <iostream>
static bool track=false;static size_t allocations=0;
void* operator new(std::size_t size){if(track)++allocations;if(auto p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
int main(){
    anc::AdaptiveNoiseCanceller lms,nlms,rls;anc::AdaptiveConfig cfg;cfg.taps=16;cfg.algorithm=anc::AdaptiveAlgorithm::Lms;lms.configure(cfg,{});cfg.algorithm=anc::AdaptiveAlgorithm::Nlms;nlms.configure(cfg,{});cfg.algorithm=anc::AdaptiveAlgorithm::Rls;rls.configure(cfg,{});
    anc::DspEngine acoustic;anc::DspConfig ac;anc::SecondaryPath s{8000,"test",{0,0,0.8f}};acoustic.configure(ac,s,s.route,true);
    std::array<float,257>x{},d{},out{};for(size_t i=0;i<x.size();++i){x[i]=float(0.1*std::sin(i*0.7));d[i]=0.7f*x[i];}
    bool okay=true;track=true;
    for(int k=0;k<20;++k){okay=lms.processBlock(x.data(),d.data(),out.data(),nullptr,x.size())&&okay;okay=nlms.processBlock(x.data(),d.data(),out.data(),nullptr,x.size())&&okay;okay=rls.processBlock(x.data(),d.data(),out.data(),nullptr,x.size())&&okay;acoustic.processBlock(x.data(),d.data(),out.data(),x.size(),false);}
    track=false;
    if(!okay||allocations){std::cerr<<"DSP failure or "<<allocations<<" processing heap allocations\n";return 1;}
    std::cout<<"PASS zero C++ heap allocations during LMS/NLMS/RLS and acoustic block processing\n";
}
