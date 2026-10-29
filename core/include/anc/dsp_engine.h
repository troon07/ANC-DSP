#pragma once
#include "fxlms.h"
#include "secondary_path.h"
#include "stability_monitor.h"
#include "dsp_config.h"
#include "adaptive.h"
namespace anc {
struct DspTelemetry {
    uint64_t samples=0,adapted=0,frozen=0,limited=0;
    double referenceEnergy=0,errorEnergy=0,outputEnergy=0;
    float outputPeak=0;
};
class DspEngine {
    FxLms controller_; StabilityMonitor safety_;SignalConditioner reference_;DspConfig config_;DspTelemetry telemetry_;
    bool armed_=false;size_t limitedRun_=0,hold_=0,plantLength_=0,growthRun_=0;double referencePower_=0,errorPower_=0,averageRate_=0,growthLimit_=0;
public:
    void configure(const DspConfig& c,const SecondaryPath& s,const std::string& actualRoute,bool stereoInput) {
        armed_=false;safety_.init(float(c.sampleRate));
        if(!stereoInput || s.route!=actualRoute || s.route=="simulation" || s.sampleRate!=c.sampleRate)
            throw std::invalid_argument("Live ANC requires calibrated stereo fixture at matching rate and route");
        if(c.sampleRate<8000||c.sampleRate>192000||!std::isfinite(c.minimumReferencePower)||c.minimumReferencePower<0||!std::isfinite(c.baselineErrorRms)||c.baselineErrorRms<0||!std::isfinite(c.maximumErrorGrowthDb)||c.maximumErrorGrowthDb<=0||c.maximumErrorGrowthDb>30)
            throw std::invalid_argument("Invalid DSP rate or monitoring configuration");
        reference_.configure({float(c.sampleRate),c.dcCutoff,c.referenceLowpass,c.conditionReference});
        controller_.configure(c.controllerTaps,s.taps,c.stepSize,c.leakage,c.epsilon,c.maximumCoefficientNorm);config_=c;plantLength_=s.taps.size();telemetry_={};limitedRun_=hold_=growthRun_=0;referencePower_=errorPower_=0;
        averageRate_=1-std::exp(-1.0/(0.05*c.sampleRate));growthLimit_=double(c.baselineErrorRms)*c.baselineErrorRms*std::pow(10.0,c.maximumErrorGrowthDb/10);
        safety_.enableAntiNoise();armed_=true;
    }
    bool isArmed() const noexcept { return armed_; }
    void stop() noexcept { armed_=false; safety_.trip(TripReason::RouteChanged); }
    TripReason tripReason()const noexcept{return safety_.tripReason();}
    const DspTelemetry& telemetry()const noexcept{return telemetry_;}
    double coefficientNorm()const noexcept{return controller_.coefficientNorm();}
    float process(float reference,float error,bool adapt=true) noexcept {
        if(!armed_) return 0;
        if(!std::isfinite(reference) || !std::isfinite(error)) { safety_.trip(TripReason::NonFiniteSample);armed_=false;return 0; }
        reference=reference_.process(reference);referencePower_+=averageRate_*(double(reference)*reference-referencePower_);errorPower_+=averageRate_*(double(error)*error-errorPower_);
        float y=controller_.output(reference);float safe=safety_.process(y);bool limited=std::abs(y)>=0.2f;
        if(limited){++limitedRun_;hold_=plantLength_;++telemetry_.limited;}else{limitedRun_=0;if(hold_)--hold_;}
        if(limitedRun_>=config_.sampleRate/10)safety_.trip(TripReason::SustainedOutputLimit);
        if(config_.baselineErrorRms>0&&telemetry_.samples>config_.sampleRate/2){if(errorPower_>growthLimit_)++growthRun_;else growthRun_=0;if(growthRun_>=config_.sampleRate/5)safety_.trip(TripReason::ErrorGrowth);}
        bool enabled=adapt&&referencePower_>=config_.minimumReferencePower&&!limited&&!hold_&&!safety_.isTripped();
        controller_.adapt(error,enabled);
        if(safety_.isTripped() || !controller_.finite()) { if(!safety_.isTripped())safety_.trip(TripReason::NonFiniteSample);armed_=false;return 0; }
        ++telemetry_.samples;if(enabled)++telemetry_.adapted;else ++telemetry_.frozen;
        telemetry_.referenceEnergy+=double(reference)*reference;telemetry_.errorEnergy+=double(error)*error;telemetry_.outputEnergy+=double(safe)*safe;telemetry_.outputPeak=std::max(telemetry_.outputPeak,std::abs(safe));
        return safe;
    }
    bool processBlock(const float*reference,const float*error,float*output,size_t frames,bool adapt=true)noexcept{
        if(frames&&(!reference||!error||!output)){safety_.trip(TripReason::InvalidFrame);armed_=false;if(output)std::fill(output,output+frames,0);return false;}
        for(size_t k=0;k<frames;++k)output[k]=process(reference[k],error[k],adapt);
        if(!armed_&&output)std::fill(output,output+frames,0);
        return armed_;
    }
};
}
