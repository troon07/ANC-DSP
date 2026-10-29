#pragma once
#include <cmath>
#include <cstddef>
#include <algorithm>
namespace anc {
struct SignalMetrics {
    double energy=0; float peak=0; size_t count=0;
    void add(float x) noexcept { energy+=double(x)*x; peak=std::max(peak,std::abs(x)); ++count; }
    double rms() const noexcept { return count?std::sqrt(energy/count):0; }
};
inline double attenuationDb(double baselineEnergy,double residualEnergy) noexcept {
    return 10*std::log10(std::max(baselineEnergy,1e-20)/std::max(residualEnergy,1e-20));
}
}
