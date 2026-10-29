#pragma once
#include "fxlms.h"
#include "secondary_path.h"
#include "stability_monitor.h"
#include "dsp_config.h"
namespace anc {
class DspEngine {
    FxLms controller_; StabilityMonitor safety_; bool armed_=false;
public:
    void configure(const DspConfig& c,const SecondaryPath& s,const std::string& actualRoute,bool stereoInput) {
        armed_=false; safety_.init(float(c.sampleRate));
        if(!stereoInput || s.route!=actualRoute || s.route=="simulation" || s.sampleRate!=c.sampleRate)
            throw std::invalid_argument("Live ANC requires calibrated stereo fixture at matching rate and route");
        controller_.configure(c.controllerTaps,s.taps,c.stepSize); safety_.enableAntiNoise(); armed_=true;
    }
    bool isArmed() const noexcept { return armed_; }
    void stop() noexcept { armed_=false; safety_.trip(TripReason::RouteChanged); }
    float process(float reference,float error) noexcept {
        if(!armed_) return 0;
        if(!std::isfinite(reference) || !std::isfinite(error)) { stop(); return 0; }
        float y=controller_.output(reference); float safe=safety_.process(y);
        controller_.adapt(error,!safety_.isTripped() && std::abs(y)<0.2f);
        if(safety_.isTripped() || !controller_.finite()) { stop(); return 0; }
        return safe;
    }
};
}
