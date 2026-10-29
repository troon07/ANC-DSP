#pragma once
#include "filter.h"
namespace anc {
// e(n)=d(n)+S(z)y(n), so the gradient update has a MINUS sign.
// Call output(x[n]) then adapt(e[n]) exactly once per sample.
class FxLms {
    FirFilter secondary_;
    std::array<float,MaxTaps> w_{},x_{},fx_{};
    size_t n_=64,pos_=0;
    float mu_=0.03f, leak_=1e-6f;
public:
    void configure(size_t n,const std::vector<float>& secondary,float mu=0.03f,float leak=1e-6f) {
        if(!n || n>MaxTaps || !std::isfinite(mu) || mu<=0 || mu>0.1f || !std::isfinite(leak) || leak<0 || leak>=1)
            throw std::invalid_argument("FxNLMS config");
        secondary_.configure(secondary); n_=n; mu_=mu; leak_=leak; reset();
    }
    void reset() noexcept { w_.fill(0); x_.fill(0); fx_.fill(0); secondary_.reset(); pos_=0; }
    float output(float ref) noexcept {
        x_[pos_]=ref; fx_[pos_]=secondary_.process(ref); float y=0; size_t j=pos_;
        for(size_t k=0;k<n_;++k) { y+=w_[k]*x_[j]; j=j?j-1:n_-1; }
        return y;
    }
    void adapt(float error,bool enabled=true) noexcept {
        if(enabled) {
            float power=1e-8f; for(size_t k=0;k<n_;++k) power+=fx_[k]*fx_[k];
            float g=mu_*error/power; size_t j=pos_;
            for(size_t k=0;k<n_;++k) { w_[k]=(1-leak_)*w_[k]-g*fx_[j]; j=j?j-1:n_-1; }
        }
        pos_=(pos_+1)%n_;
    }
    std::vector<float> weights() const { return {w_.begin(),w_.begin()+n_}; }
    bool finite() const noexcept { for(size_t k=0;k<n_;++k) if(!std::isfinite(w_[k])) return false; return true; }
};
}
