#pragma once
#include "lms.h"
#include <numeric>
namespace anc {
struct CalibrationResult { std::vector<float> taps; double validationNmseDb=0; size_t peakDelay=0; };
inline CalibrationResult identifySecondaryPath(const std::vector<float>& probe,const std::vector<float>& response,size_t taps) {
    if(probe.size()!=response.size() || probe.size()<taps*20 || !taps) throw std::invalid_argument("Calibration data length");
    double energy=0; for(float x:probe) { if(!std::isfinite(x)) throw std::invalid_argument("Probe nonfinite"); energy+=x*x; }
    for(float x:response) if(!std::isfinite(x)) throw std::invalid_argument("Response nonfinite");
    if(energy<1e-8) throw std::invalid_argument("Silent probe");
    Nlms model; model.configure(taps,0.3f); size_t split=probe.size()*3/4;
    for(size_t k=0;k<split;++k) model.process(probe[k],response[k]);
    auto h=model.weights(); FirFilter f; f.configure(h); double err=0,desired=0;
    for(size_t k=0;k<probe.size();++k) { float e=response[k]-f.process(probe[k]); if(k>=split) {err+=e*e; desired+=response[k]*response[k];} }
    if(desired<1e-10) throw std::invalid_argument("Silent measured response");
    size_t peak=0; for(size_t k=1;k<taps;++k) if(std::abs(h[k])>std::abs(h[peak])) peak=k;
    return {h,10*std::log10(std::max(err,1e-20)/desired),peak};
}
}
