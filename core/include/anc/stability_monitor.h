#pragma once
#include <cmath>
#include <algorithm>
#include <cstddef>
namespace anc {
enum class TripReason { None,NonFiniteSample,ExcessiveClipping,RouteChanged,SustainedOutputLimit,ErrorGrowth,InvalidFrame };
class StabilityMonitor {
    bool enabled_=false,tripped_=false;
    TripReason reason_=TripReason::None;
    size_t clips_=0;
public:
    void init(float) noexcept { enabled_=tripped_=false; reason_=TripReason::None; clips_=0; }
    void enableAntiNoise() noexcept { if(!tripped_) enabled_=true; }
    void trip(TripReason r) noexcept { reason_=r; tripped_=true; enabled_=false; }
    bool isTripped() const noexcept { return tripped_; }
    TripReason tripReason() const noexcept { return reason_; }
    static float softClip(float x) noexcept { return std::tanh(x); }
    float process(float x) noexcept {
        if(!std::isfinite(x)) trip(TripReason::NonFiniteSample);
        else if(std::abs(x)>=0.95f) { if(++clips_>=16) trip(TripReason::ExcessiveClipping); }
        else clips_=0;
        return enabled_ && !tripped_ ? std::clamp(x,-0.2f,0.2f):0;
    }
    void processOutputBlock(float* x,size_t n) noexcept {
        for(size_t k=0;k<n;++k) x[k]=process(x[k]);
        if(tripped_) std::fill(x,x+n,0.0f);
    }
};
}
