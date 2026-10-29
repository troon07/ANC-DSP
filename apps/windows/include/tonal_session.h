#pragma once
#include "wasapi_backend.h"
#include "anc/tonal.h"
#include <filesystem>
#include <fstream>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
namespace anc {
inline std::string jsonString(const std::string& s){std::ostringstream out;out<<'"';for(unsigned char c:s){if(c=='"'||c=='\\')out<<'\\'<<c;else if(c<32)out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<unsigned(c)<<std::dec;else out<<c;}out<<'"';return out.str();}
inline void writePcm16(const std::filesystem::path& path,const std::vector<float>& samples,size_t n,unsigned rate){
    std::ofstream out(path,std::ios::binary);if(!out)throw std::runtime_error("Cannot save WAV");
    auto u16=[&](uint16_t x){out.write(reinterpret_cast<const char*>(&x),2);};auto u32=[&](uint32_t x){out.write(reinterpret_cast<const char*>(&x),4);};
    out.write("RIFF",4);u32(uint32_t(36+n*2));out.write("WAVEfmt ",8);u32(16);u16(1);u16(1);u32(rate);u32(rate*2);u16(2);u16(16);out.write("data",4);u32(uint32_t(n*2));
    for(size_t i=0;i<n;++i){int16_t x=int16_t(std::lround(std::clamp(samples[i],-1.0f,1.0f)*32767));out.write(reinterpret_cast<const char*>(&x),2);}if(!out)throw std::runtime_error("WAV write failed");
}
class TonalSession {
    WasapiBackend audio_;TonalController controller_;std::vector<float> mic_,drive_;
    AudioDeviceInfo input_,output_;std::filesystem::path folder_;bool saved_=false;
    std::atomic<bool> finished_{false};std::atomic<int> stage_{0};std::atomic<uint64_t> count_{0};
    std::atomic<double> rms_{0},tone_{0},callbackUs_{0},maxCallbackUs_{0};
public:
    ~TonalSession(){audio_.closeStream();}
    void start(const AudioDeviceInfo& input,const AudioDeviceInfo& output,double frequency,const std::filesystem::path& parent){
        if(audio_.isRunning())throw std::runtime_error("A session is already active");
        input_=input;output_=output;if(input.default_sample_rate!=output.default_sample_rate)throw std::runtime_error("Select built-in 48 kHz microphone and 48 kHz headphone media output, not the headset communications microphone");
        controller_.configure(input.default_sample_rate,frequency);mic_.assign(controller_.totalSamples(),0);drive_.assign(mic_.size(),0);
        folder_=parent/("tonal-"+std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()));
        std::filesystem::create_directories(folder_);finished_=false;stage_=0;count_=0;rms_=tone_=callbackUs_=maxCallbackUs_=0;saved_=false;
        audio_.setTonalMode(true);audio_.setCallback([this](const AudioFrame& in,AudioFrame& out){
            auto begin=std::chrono::steady_clock::now();
            for(size_t k=0;k<in.frames;++k){float y=0;if(!controller_.done()){auto n=controller_.sampleCount();float x=in.data[2*k];y=controller_.process(x);mic_[n]=std::isfinite(x)?x:0;drive_[n]=y;}out.data[2*k]=y;out.data[2*k+1]=0;}
            auto point=controller_.live();rms_=std::sqrt(point.power);tone_=point.tone;stage_=controller_.stage();count_=controller_.sampleCount();
            double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();callbackUs_=us;maxCallbackUs_=std::max(maxCallbackUs_.load(),us);
            if(controller_.done())finished_=true;
        });
        if(!audio_.openStream(input.id,output.id,input.default_sample_rate,128)){audio_.closeStream();throw std::runtime_error("Cannot open RAW float microphone and headphone stream at matching native rate. Check Windows microphone access and select the built-in microphone, not Bluetooth capture.");}
    }
    bool finished()const{return finished_.load();}bool running()const{return audio_.isRunning();}
    int stage()const{return stage_.load();}double seconds()const{return double(count_.load())/input_.default_sample_rate;}
    double rms()const{return rms_.load();}double tonePower()const{return tone_.load();}double callbackUs()const{return callbackUs_.load();}
    const std::filesystem::path& folder()const{return folder_;}
    std::string finish(bool userStop=false){
        bool streamFault=!audio_.isRunning()&&!finished_.load();audio_.closeStream();
        if(!controller_.done())controller_.abort(streamFault?ToneFault::AudioRoute:ToneFault::Interrupted);
        if(saved_)return "Session already saved";saved_=true;size_t n=size_t(controller_.sampleCount());
        writePcm16(folder_/"microphone.wav",mic_,n,controller_.sampleRate());writePcm16(folder_/"control.wav",drive_,n,controller_.sampleRate());
        std::ofstream table(folder_/"stages.csv");table<<"stage,tone_power,total_power,peak,sample_count\n"<<std::setprecision(12);
        for(size_t i=0;i<8;++i){auto p=controller_.points()[i];table<<i<<','<<p.tone<<','<<p.power<<','<<p.peak<<','<<p.count<<'\n';}
        bool complete=controller_.complete();double before=(controller_.points()[5].tone+controller_.points()[7].tone)/2;
        auto on=controller_.points()[6];double floor=std::max(1.0/(12*32767.0*32767.0),on.power-on.tone)*2/std::max<uint64_t>(1,on.count);
        bool floorLimited=complete&&on.tone<floor;
        double attenuation=complete?TonalController::dbRatio(before,std::max(on.tone,floor)):0;
        bool observed=complete&&attenuation>=3;
        std::ofstream meta(folder_/"session.json");meta<<std::setprecision(12)<<"{\n\"schema_version\":1,\n\"provenance\":\"physical_microphone_recording\",\n\"platform\":\"windows\",\n\"sample_rate\":"<<controller_.sampleRate()<<",\n\"frequency_hz\":"<<controller_.frequency()<<",\n\"stage_seconds\":4,\n\"settle_seconds\":2,\n\"measurement_tail_seconds\":0.1,\n\"sample_count\":"<<n<<",\n\"complete\":"<<(complete?"true":"false")<<",\n\"reduction_observed\":"<<(observed?"true":"false")<<",\n\"fault\":"<<jsonString(toneFaultText(controller_.fault()))<<",\n\"input_name\":"<<jsonString(input_.name)<<",\n\"output_name\":"<<jsonString(output_.name)<<",\n\"input_id\":"<<jsonString(input_.id)<<",\n\"output_id\":"<<jsonString(output_.id)<<",\n\"probe_amplitude\":0.02,\n\"candidate_real\":"<<controller_.candidate().real()<<",\n\"candidate_imag\":"<<controller_.candidate().imag()<<",\n\"model_valid\":"<<(controller_.modelValid()?"true":"false")<<",\n\"predicted_tone_power\":"<<controller_.predictedTonePower()<<",\n\"calibration_fit_error\":"<<controller_.fitError()<<",\n\"buffer_budget_estimate_ms\":"<<audio_.getEstimatedRoundtripLatencyMs()<<",\n\"max_callback_us\":"<<maxCallbackUs_.load()<<",\n\"stream_faults\":"<<audio_.getUnderrunCount()<<",\n\"raw_capture_requested\":true,\n\"measured_round_trip_latency_ms\":null,\n\"scope\":\"single tone at microphone position; not verified at ear\"\n}\n";
        if(!meta||!table)throw std::runtime_error("Cannot write session metadata");
        std::ostringstream text;text<<std::fixed<<std::setprecision(2);
        if(complete)text<<(observed?"Measured narrowband reduction: ":"No >=3 dB reduction demonstrated: ")<<attenuation<<" dB at the microphone."<<(floorLimited?" (Target tone below estimated measurement floor.)":"");
        else text<<"Experiment incomplete: "<<toneFaultText(controller_.fault());
        text<<"\nSaved "<<n<<" real microphone samples to "<<folder_.u8string();(void)userStop;return text.str();
    }
};
}
