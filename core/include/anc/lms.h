#pragma once
#include "filter.h"
namespace anc {
class Nlms {
    std::array<float,MaxTaps> w_{}, x_{};
    size_t n_=32, pos_=0;
    float mu_=0.1f;
public:
    void configure(size_t n,float mu=0.1f) {
        if(n==0 || n>MaxTaps || !std::isfinite(mu) || mu<=0 || mu>=2) throw std::invalid_argument("NLMS config");
        n_=n; mu_=mu; w_.fill(0); x_.fill(0); pos_=0;
    }
    float process(float x,float desired) noexcept {
        x_[pos_]=x; float y=0, energy=1e-8f; size_t j=pos_;
        for(size_t k=0;k<n_;++k) { y+=w_[k]*x_[j]; energy+=x_[j]*x_[j]; j=j?j-1:n_-1; }
        float e=desired-y,g=mu_*e/energy; j=pos_;
        for(size_t k=0;k<n_;++k) { w_[k]+=g*x_[j]; j=j?j-1:n_-1; }
        pos_=(pos_+1)%n_; return e;
    }
    std::vector<float> weights() const { return {w_.begin(),w_.begin()+n_}; }
};
}
