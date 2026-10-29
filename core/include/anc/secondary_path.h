#pragma once
#include "filter.h"
#include <fstream>
#include <string>
namespace anc {
struct SecondaryPath {
    unsigned sampleRate=0;
    std::string route;
    std::vector<float> taps;
    // Plain text, versioned and route-bound. Simulation profiles cannot arm live output.
    static SecondaryPath load(const std::string& file) {
        std::ifstream in(file); SecondaryPath s; std::string magic; size_t n=0;
        if(!(in>>magic>>s.sampleRate>>s.route>>n) || magic!="ANC_PROFILE_V1" || !s.sampleRate || !n || n>MaxTaps)
            throw std::runtime_error("Invalid secondary path profile");
        s.taps.resize(n); double energy=0;
        for(float& v:s.taps) { if(!(in>>v) || !std::isfinite(v)) throw std::runtime_error("Invalid profile taps"); energy+=v*v; }
        std::string extra; if(in>>extra || energy<1e-10) throw std::runtime_error("Invalid profile data"); return s;
    }
    void save(const std::string& file) const {
        std::ofstream out(file); if(!out) throw std::runtime_error("Cannot write profile");
        out.precision(10); out<<"ANC_PROFILE_V1 "<<sampleRate<<' '<<route<<' '<<taps.size()<<'\n';
        for(float v:taps) out<<v<<'\n';
    }
};
}
