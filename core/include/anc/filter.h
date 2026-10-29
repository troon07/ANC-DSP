#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace anc {
constexpr size_t MaxTaps = 4096;
class FirFilter {
    std::array<float,MaxTaps> h_{}, x_{};
    size_t length_=1, pos_=0;
public:
    void configure(const std::vector<float>& h) {
        if(h.empty() || h.size()>MaxTaps) throw std::invalid_argument("FIR length");
        for(float v:h) if(!std::isfinite(v)) throw std::invalid_argument("FIR coefficients");
        length_=h.size(); std::copy(h.begin(),h.end(),h_.begin()); reset();
    }
    void reset() noexcept { x_.fill(0); pos_=0; }
    float process(float x) noexcept {
        x_[pos_]=x; float y=0; size_t j=pos_;
        for(size_t k=0;k<length_;++k) { y+=h_[k]*x_[j]; j=j?j-1:length_-1; }
        pos_=(pos_+1)%length_; return y;
    }
};
class DcBlocker {
    float previous_=0, state_=0, r_=0.995f;
public:
    void configure(float fs, float cutoff=10) {
        if(!std::isfinite(fs)||!std::isfinite(cutoff)||fs<=0||cutoff<=0||cutoff>=fs/2) throw std::invalid_argument("DC blocker cutoff");
        r_=std::exp(-6.28318530718f*cutoff/fs); reset();
    }
    void reset() noexcept { previous_=state_=0; }
    float process(float x) noexcept { float y=x-previous_+r_*state_; previous_=x; state_=y; return y; }
};
class Biquad {
    float b0_=1,b1_=0,b2_=0,a1_=0,a2_=0,z1_=0,z2_=0;
public:
    void lowpass(float fs,float fc,float q=0.70710678f) {
        if(!std::isfinite(fs)||!std::isfinite(fc)||!std::isfinite(q)||!(fs>0 && fc>0 && fc<fs/2 && q>0)) throw std::invalid_argument("Biquad cutoff");
        float w=6.28318530718f*fc/fs,c=std::cos(w),a=std::sin(w)/(2*q),d=1+a;
        b0_=(1-c)/(2*d); b1_=2*b0_; b2_=b0_; a1_=-2*c/d; a2_=(1-a)/d; reset();
    }
    void reset() noexcept { z1_=z2_=0; }
    float process(float x) noexcept { float y=b0_*x+z1_; z1_=b1_*x-a1_*y+z2_; z2_=b2_*x-a2_*y; return y; }
};
}
