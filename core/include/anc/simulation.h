#pragma once
#include "fxlms.h"
#include "metrics.h"
#include <string>
#include <cstdint>
namespace anc {
struct SimulationSummary { double attenuation=0,earlyAttenuation=0,peak=0; size_t clips=0; };
inline const std::vector<std::string>& scenarioNames() {
    static const std::vector<std::string> names={"broadband","hum_drone","fan","uncorrelated","mismatch","wired_delay","wireless_delay","path_change"};return names;
}
template<class Sink> SimulationSummary simulate(const std::string& name,Sink sink) {
    if(std::find(scenarioNames().begin(),scenarioNames().end(),name)==scenarioNames().end()) throw std::invalid_argument("Unknown simulation scenario");
    constexpr int fs=8000,N=48000;
    size_t delay=name=="wired_delay"?40:name=="wireless_delay"?400:8;
    std::vector<float> p(27,0),s(delay+3,0),estimate;
    p[24]=0.65f;p[25]=0.15f;p[26]=-0.05f;
    s[delay]=0.8f;s[delay+1]=-0.15f;s[delay+2]=0.05f;estimate=s;
    if(name=="mismatch") {estimate[delay]=0.65f; estimate[delay+1]=-0.05f;}
    FirFilter primary,secondary;primary.configure(p);secondary.configure(s);
    FxLms controller;controller.configure(64,estimate,0.02f);
    uint32_t state=42; auto random=[&](){state=1664525u*state+1013904223u;return float(double(state)/4294967296.0*2-1);};
    double baseline=0,residual=0,earlyD=0,earlyE=0; SimulationSummary result; float colored=0;
    for(int k=0;k<N;++k) {
        float white=random(),independent=random(); colored=0.85f*colored+0.15f*white;
        double t=double(k)/fs; float x=0.08f*white;
        if(name=="hum_drone") x=float(0.055*std::sin(6.283185307179586*120*t)+0.025*std::sin(6.283185307179586*240*t)+0.015*colored);
        if(name=="fan") x=0.12f*colored;
        if(name=="path_change" && k==N/2) {s[delay]=0.6f; s[delay+1]=-0.08f; secondary.configure(s);}
        float d=primary.process(x);if(name=="uncorrelated")d+=0.04f*independent;
        float raw=controller.output(x),y=std::clamp(raw,-0.2f,0.2f),e=d+secondary.process(y);
        if(!std::isfinite(e)||!controller.finite())throw std::runtime_error("Simulation diverged");
        bool clipped=std::abs(raw)>=0.2f;controller.adapt(e,!clipped);result.clips+=clipped; result.peak=std::max(result.peak,double(std::abs(y)));
        if(k>=N*3/4) {baseline+=d*d;residual+=e*e;}
        if(k<fs/2) {earlyD+=d*d;earlyE+=e*e;}
        sink(k,x,d,y,e);
    }
    result.attenuation=attenuationDb(baseline,residual);result.earlyAttenuation=attenuationDb(earlyD,earlyE);return result;
}
}
