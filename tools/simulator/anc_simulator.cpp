#include "anc/simulation.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
int main(int argc,char**argv) {
    try {
        std::string scenario=argc>1?argv[1]:"all",directory=argc>2?argv[2]:"results/cpp";
        std::filesystem::create_directories(directory); std::ofstream summary(directory+"/summary.csv");
        if(!summary) throw std::runtime_error("Cannot write summary");
        summary<<"scenario,attenuation_db,early_attenuation_db,output_peak,clipped_samples,sample_rate,samples\n";
        for(const auto& name:anc::scenarioNames()) {
            if(scenario!="all" && name!=scenario)continue;
            std::ofstream trace(directory+"/"+name+".csv");if(!trace)throw std::runtime_error("Cannot write trace");
            trace<<"sample,reference,baseline,control,residual\n"<<std::setprecision(10);
            auto r=anc::simulate(name,[&](int k,float x,float d,float y,float e){trace<<k<<','<<x<<','<<d<<','<<y<<','<<e<<'\n';});
            summary<<name<<','<<r.attenuation<<','<<r.earlyAttenuation<<','<<r.peak<<','<<r.clips<<",8000,48000\n";
            std::cout<<name<<": "<<r.attenuation<<" dB (simulated, final 1.5 seconds), peak="<<r.peak<<'\n';
        }
        if(scenario!="all" && std::find(anc::scenarioNames().begin(),anc::scenarioNames().end(),scenario)==anc::scenarioNames().end())throw std::invalid_argument("Unknown scenario");
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;
}
