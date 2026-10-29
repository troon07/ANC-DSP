#include "wasapi_backend.h"
#include "anc/dsp_engine.h"
#include "anc/simulation.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
int main(int argc,char**argv){
 try {
    anc::WasapiBackend audio;std::string mode=argc>1?argv[1]:"--help";
    if(mode=="--devices"){
        for(auto direction:{0,1}){auto devices=direction?audio.enumerateRenderDevices():audio.enumerateCaptureDevices();std::cout<<(direction?"OUTPUT":"INPUT")<<'\n';
            for(auto&d:devices)std::cout<<d.name<<" | "<<d.default_sample_rate<<" Hz | "<<d.channels<<" channels\nID: "<<d.id<<'\n';}return 0;
    }
    if(mode=="--demo"){auto r=anc::simulate("hum_drone",[](int,float,float,float,float){});std::cout<<r.attenuation<<" dB simulated attenuation; no hardware output\n";return 0;}
    if((mode=="--fixture" || mode=="--record-probe") && argc==7){
        // Explicit fixture operation only: profile/recording, capture ID, render ID, sample rate, duration.
        unsigned rate=std::stoul(argv[5]),seconds=std::stoul(argv[6]);if(!rate||seconds<1||seconds>60)throw std::invalid_argument("Duration must be 1..60 seconds");
        anc::DspEngine engine;anc::DspConfig config;config.sampleRate=rate;config.controllerTaps=256;config.stepSize=0.02f;
        std::string route=std::string(argv[3])+"|"+argv[4];
        if(mode=="--fixture")engine.configure(config,anc::SecondaryPath::load(argv[2]),route,true);
        size_t count=0,limit=size_t(rate)*seconds;std::vector<float>probe(limit),response(limit);uint32_t random=42;
        audio.setCallback([&](const anc::AudioFrame&in,anc::AudioFrame&out){
            for(size_t k=0;k<in.frames;++k){float y=0;
                if(count<limit){if(mode=="--record-probe"){random=1664525u*random+1013904223u;y=float((double(random)/4294967296.0*2-1)*0.02);probe[count]=y;response[count]=in.data[2*k+1];}else y=engine.process(in.data[2*k],in.data[2*k+1]);++count;}
                out.data[2*k]=y;out.data[2*k+1]=0;
            }
        });
        if(!audio.openStream(argv[3],argv[4],rate,128))throw std::runtime_error("Cannot open native stereo float fixture at requested rate");
        std::cout<<"Fixture running. Buffer budget estimate "<<audio.getEstimatedRoundtripLatencyMs()<<" ms (not measured round-trip).\n";
        auto end=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
        while(audio.isRunning()&&std::chrono::steady_clock::now()<end)std::this_thread::sleep_for(std::chrono::milliseconds(20));
        bool stoppedEarly=!audio.isRunning();audio.closeStream();bool controllerFailed=(mode=="--fixture" && !engine.isArmed());engine.stop();
        if(controllerFailed)throw std::runtime_error("DSP safety trip; controller output muted");
        if(stoppedEarly)throw std::runtime_error("Stream stopped after discontinuity or device error; calibration timing invalid");
        if(mode=="--record-probe"){
            std::ofstream f(argv[2]);if(!f)throw std::runtime_error("Cannot write recording");f<<"probe,response\n";for(size_t i=0;i<count;++i)f<<probe[i]<<','<<response[i]<<'\n';
            std::cout<<"Route token for calibration: "<<route<<'\n';
        }
        std::cout<<"Stopped; stream faults "<<audio.getUnderrunCount()<<'\n';return 0;
    }
    std::cout<<"Universal ANC research prototype\n--devices : list physical routes\n--demo : silent DSP simulation\n"
        <<"--record-probe recording.csv capture_ID render_ID sample_rate seconds\n"
        <<"--fixture calibrated.profile capture_ID render_ID sample_rate seconds\n"
        <<"Live modes require a bench fixture: input 0 reference, input 1 error mic, output 0 control.\n"
        <<"Use the SAME USB interface for capture/render. Bluetooth: diagnostics/simulation only.\n";
    return mode=="--help"?0:1;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
