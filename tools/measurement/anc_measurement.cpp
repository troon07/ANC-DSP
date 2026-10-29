#include "anc/calibration.h"
#include "anc/secondary_path.h"
#include <fstream>
#include <sstream>
#include <iostream>
int main(int argc,char**argv) {
    if(argc!=7){std::cout<<"Usage: anc_measurement probe_response.csv sample_rate taps route_token output.profile report.csv\n"
        <<"CSV header: probe,response. Include full playback/capture delay in aligned samples.\n";return argc==1?0:1;}
    try {
        std::ifstream in(argv[1]); if(!in)throw std::runtime_error("Cannot read recording");
        std::string line;std::getline(in,line);if(line!="probe,response" && line!="probe,response\r")throw std::runtime_error("Expected CSV header probe,response");
        std::vector<float>x,y;
        while(std::getline(in,line)) {if(line.empty())continue;std::replace(line.begin(),line.end(),',',' ');std::istringstream row(line);float a,b;std::string extra;if(!(row>>a>>b)||row>>extra)throw std::runtime_error("Invalid recording row");x.push_back(a);y.push_back(b);}
        auto r=anc::identifySecondaryPath(x,y,std::stoul(argv[3]));
        if(r.validationNmseDb>-15)throw std::runtime_error("Calibration validation failed: NMSE must be below -15 dB");
        anc::SecondaryPath s{unsigned(std::stoul(argv[2])),argv[4],r.taps};s.save(argv[5]);
        std::ofstream report(argv[6]); if(!report)throw std::runtime_error("Cannot write calibration report");
        report<<"validation_nmse_db,peak_delay_samples\n"<<r.validationNmseDb<<','<<r.peakDelay<<'\n';
        std::cout<<"Held-out NMSE "<<r.validationNmseDb<<" dB; peak delay "<<r.peakDelay<<" samples\n";
    }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}return 0;
}
