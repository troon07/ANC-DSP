#include "anc/adaptive.h"
#include "anc/metrics.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <chrono>
using namespace anc;
AdaptiveAlgorithm algorithm(const std::string& name){if(name=="lms")return AdaptiveAlgorithm::Lms;if(name=="nlms")return AdaptiveAlgorithm::Nlms;if(name=="rls")return AdaptiveAlgorithm::Rls;throw std::invalid_argument("Algorithm must be lms, nlms or rls");}
AdaptiveConfig settings(const std::string& name){AdaptiveConfig c;c.algorithm=algorithm(name);c.taps=16;c.step=name=="lms"?0.1f:0.05f;c.forgetting=0.999;return c;}
void benchmark(const std::filesystem::path& directory){
    std::filesystem::create_directories(directory);std::ofstream summary(directory/"summary.csv");if(!summary)throw std::runtime_error("Cannot create summary");
    summary<<"scenario,algorithm,input_snr_db,output_snr_db,snr_improvement_db,late_mse,early_mse,dropout_mse,desired_signal_gain,processing_us_per_sample\n"<<std::setprecision(12);
    for(const std::string scenario:{"stationary","path_change","uncorrelated","reference_leakage","reference_dropout"})for(const std::string name:{"lms","nlms","rls"}){
        AdaptiveNoiseCanceller canceller;canceller.configure(settings(name),{});SignalConditioner target;target.configure({});FirFilter noise;noise.configure({0.75f,-0.3f,0,0.2f,0,0,-0.08f});
        uint32_t state=73;auto random=[&](){state=1664525u*state+1013904223u;return float(double(state)/4294967296.0*2-1);};float memory=0;
        constexpr size_t N=32000;double cleanPower=0,inputError=0,outputError=0,early=0,drop=0,cross=0;size_t dropCount=0;double micros=0;
        std::ofstream trace(directory/(scenario+"_"+name+".csv"));if(!trace)throw std::runtime_error("Cannot create trace");trace<<"sample,reference,primary,clean,target,conditioned_primary,estimate,residual\n"<<std::setprecision(10);
        for(size_t k=0;k<N;++k){float white=random(),independent=random();memory=0.35f*memory+0.12f*white;float reference=memory+0.02f;double t=double(k)/8000;
            float clean=float((0.8+0.2*std::sin(6.283185307179586*0.7*t))*(0.035*std::sin(6.283185307179586*370*t)+0.02*std::sin(6.283185307179586*720*t)));
            if(scenario=="path_change"&&k==N/2)noise.configure({0.35f,0.2f,0,-0.15f,0,0,0.1f});
            float primary=clean+noise.process(memory)+0.015f;
            if(scenario=="uncorrelated")primary+=0.04f*independent;
            if(scenario=="reference_leakage")reference+=1.5f*clean;
            if(scenario=="reference_dropout"&&k>=12000&&k<20000)reference=0;
            auto begin=std::chrono::steady_clock::now();auto v=canceller.process(reference,primary);micros+=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();if(!v.valid)throw std::runtime_error("Adaptive filter fault");
            float truth=target.process(clean),conditioned=v.residual+v.estimate;double e=double(v.residual)-truth,ie=double(conditioned)-truth;
            if(k>=24000){cleanPower+=truth*truth;inputError+=ie*ie;outputError+=e*e;cross+=double(v.residual)*truth;}
            if(k<2000)early+=e*e;
            if(k>=12000&&k<20000){drop+=e*e;++dropCount;}
            trace<<k<<','<<reference<<','<<primary<<','<<clean<<','<<truth<<','<<conditioned<<','<<v.estimate<<','<<v.residual<<'\n';
        }
        double input=attenuationDb(cleanPower,inputError),output=attenuationDb(cleanPower,outputError);
        summary<<scenario<<','<<name<<','<<input<<','<<output<<','<<output-input<<','<<outputError/8000<<','<<early/2000<<','<<drop/dropCount<<','<<cross/cleanPower<<','<<micros/N<<'\n';
        std::cout<<scenario<<' '<<name<<": "<<input<<" -> "<<output<<" dB synthetic SNR\n";
    }
}
void processFile(const std::string& file,const std::string& name,const std::string& output){
    std::error_code ec;
    if(std::filesystem::equivalent(file,output,ec)||std::filesystem::weakly_canonical(file)==std::filesystem::weakly_canonical(output))throw std::invalid_argument("Input and output must be different files");
    std::ifstream in(file);if(!in)throw std::runtime_error("Cannot open input");std::string line;std::getline(in,line);if(line!="reference,primary"&&line!="reference,primary\r")throw std::runtime_error("Expected CSV header reference,primary");
    std::ofstream out(output);if(!out)throw std::runtime_error("Cannot open output");
    AdaptiveNoiseCanceller canceller;canceller.configure(settings(name),{});out<<"estimate,residual\n"<<std::setprecision(10);size_t count=0;
    while(std::getline(in,line)){if(line.empty())continue;std::replace(line.begin(),line.end(),',',' ');std::istringstream row(line);float x,d;std::string extra;if(!(row>>x>>d)||row>>extra)throw std::runtime_error("Invalid CSV row");auto v=canceller.process(x,d);if(!v.valid)throw std::runtime_error("Nonfinite input or filter fault");out<<v.estimate<<','<<v.residual<<'\n';++count;}
    if(!count)throw std::runtime_error("Empty input");if(!out)throw std::runtime_error("Output write failed");std::cout<<"Processed "<<count<<" frames at assumed 8000 Hz. Digital residual, not acoustic anti-noise; SNR unknown without a clean target.\n";
}
int main(int argc,char**argv){try{
    if(argc==3&&std::string(argv[1])=="--benchmark")benchmark(argv[2]);
    else if(argc==5&&std::string(argv[1])=="--process")processFile(argv[2],argv[3],argv[4]);
    else{std::cout<<"anc_adaptive --benchmark output_directory\nanc_adaptive --process input.csv lms|nlms|rls output.csv\nCSV is synchronized reference,primary at 8 kHz.\n";return argc==1?0:1;}
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
