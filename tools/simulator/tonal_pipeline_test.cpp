#include "anc/tonal.h"
#include "tonal_session.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{
    std::filesystem::path folder=argc>1?argv[1]:"results/tonal_pipeline_test";std::filesystem::create_directories(folder);
    anc::TonalController controller;controller.configure(8000,300);std::vector<float> mic(controller.totalSamples()),output(mic.size());std::array<float,800> delay{};size_t pos=0;
    for(size_t n=0;n<mic.size()&&!controller.done();++n){double t=double(n)/8000;mic[n]=float(0.03*std::cos(6.283185307179586*300*t+0.6)+0.001*std::cos(6.283185307179586*750*t)+0.7*delay[pos]);output[n]=controller.process(mic[n]);delay[pos]=output[n];pos=(pos+1)%delay.size();}
    if(!controller.complete())throw std::runtime_error(anc::toneFaultText(controller.fault()));
    anc::writePcm16(folder/"microphone.wav",mic,mic.size(),8000);anc::writePcm16(folder/"control.wav",output,output.size(),8000);
    std::ofstream meta(folder/"session.json");meta<<std::setprecision(14)<<"{\"schema_version\":1,\"provenance\":\"synthetic_pipeline_test\",\"platform\":\"cpp_test\",\"sample_rate\":8000,\"frequency_hz\":300,\"stage_seconds\":4,\"settle_seconds\":2,\"measurement_tail_seconds\":0.1,\"sample_count\":"<<mic.size()<<",\"complete\":true,\"fault\":\"none\",\"input_name\":\"synthetic plant\",\"output_name\":\"synthetic control\",\"probe_amplitude\":0.02,\"candidate_real\":"<<controller.candidate().real()<<",\"candidate_imag\":"<<controller.candidate().imag()<<",\"model_valid\":true,\"raw_capture_requested\":true,\"stream_faults\":0,\"max_callback_us\":0}\n";
    std::cout<<"SYNTHETIC pipeline fixture saved to "<<folder<<". Not measured ANC.\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
