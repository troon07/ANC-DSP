#pragma once
#include <array>
#include <cmath>
#include <complex>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
namespace anc {
struct TonePoint { double tone=0,power=0,peak=0; uint64_t count=0; };
class ToneMeter {
    double sx=0,sxx=0,sc=0,ss=0,scc=0,sss=0,scs=0,sxc=0,sxs=0,peak=0;
    uint64_t n=0;
public:
    void add(double x,double c,double s) noexcept {
        sx+=x;sxx+=x*x;sc+=c;ss+=s;scc+=c*c;sss+=s*s;scs+=c*s;sxc+=x*c;sxs+=x*s;peak=std::max(peak,std::abs(x));++n;
    }
    TonePoint value() const noexcept {
        if(n<16)return {};
        double cc=scc-sc*sc/n, sn=sss-ss*ss/n, cs=scs-sc*ss/n;
        double xc=sxc-sx*sc/n, xs=sxs-sx*ss/n,det=cc*sn-cs*cs;
        if(det<1e-10)return {};
        double a=(xc*sn-xs*cs)/det,b=(xs*cc-xc*cs)/det;
        return {(a*a+b*b)/2,std::max(0.0,sxx/n-(sx/n)*(sx/n)),peak,n};
    }
};
enum class ToneFault { None,NonFinite,InputClipping,WeakTone,InconsistentCalibration,UnidentifiablePath,ExcessiveOutput,SourceDrift,Amplification,Interrupted,AudioRoute };
inline const char* toneFaultText(ToneFault f) noexcept {
    switch(f){
    case ToneFault::None:return "none";case ToneFault::NonFinite:return "nonfinite microphone sample";
    case ToneFault::InputClipping:return "microphone clipping";case ToneFault::WeakTone:return "target tone missing or too weak";
    case ToneFault::InconsistentCalibration:return "source phase/gain changed during calibration";
    case ToneFault::UnidentifiablePath:return "earbud response cannot be identified";
    case ToneFault::ExcessiveOutput:return "required output exceeds 0.08 FS cap; move earbud closer";
    case ToneFault::SourceDrift:return "ANC-off tone level changed by more than 1 dB";
    case ToneFault::Amplification:return "anti-tone increased measured target power";
    case ToneFault::Interrupted:return "user stopped before experiment completed";
    case ToneFault::AudioRoute:return "audio stream fault or route change";
    }return "unknown";
}
inline const char* toneStageName(int stage) noexcept {
    static const char* names[]={"Baseline OFF","Probe 0 degrees","Probe 90 degrees","Probe 180 degrees","Probe 270 degrees","Validation OFF","Candidate ON","Recheck OFF","Finished"};
    return names[std::clamp(stage,0,8)];
}
class TonalController {
    unsigned fs_=48000;double frequency_=300;
    uint64_t stageFrames_=192000,settle_=96000,ramp_=960,tail_=4800,position_=0;
    ToneMeter meter_,window_;uint64_t windowCount_=0;
    std::array<TonePoint,8> points_{};TonePoint live_{};
    std::complex<double> candidate_{0,0};double predictedTone_=0,fitError_=0;
    ToneFault fault_=ToneFault::None;bool model_=false;
    void fit() noexcept {
        double base=points_[0].tone,a=probeAmplitude();
        double p0=points_[1].tone,p1=points_[2].tone,p2=points_[3].tone,p3=points_[4].tone;
        double mean=(p0+p1+p2+p3)/4;
        fitError_=std::abs(p0+p2-p1-p3)/(4*std::max(mean,1e-20));
        if(base<1e-9 || base<std::max(1e-12,points_[0].power)*0.05){abort(ToneFault::WeakTone);return;}
        if(fitError_>0.1){abort(ToneFault::InconsistentCalibration);return;}
        double B=(mean-base)/(a*a);
        if(B<=std::max(1e-12,base*1e-4/(a*a))){abort(ToneFault::UnidentifiablePath);return;}
        std::complex<double> G{(p0-p2)/(4*a),(p3-p1)/(4*a)};
        candidate_=-std::conj(G)/B;
        predictedTone_=base-std::norm(G)/B;
        if(predictedTone_<-0.05*base || predictedTone_>0.5*base){abort(ToneFault::InconsistentCalibration);return;}
        predictedTone_=std::max(0.0,predictedTone_);
        if(!std::isfinite(std::abs(candidate_)) || std::abs(candidate_)>0.08){abort(ToneFault::ExcessiveOutput);return;}
        model_=true;
    }
public:
    static constexpr double probeAmplitude(){return 0.02;}
    void configure(unsigned fs,double frequency,double stageSeconds=4,double settleSeconds=2) {
        if(fs<8000 || fs>192000 || !std::isfinite(frequency) || frequency<100 || frequency>1000 || frequency>=fs/4 || stageSeconds<1 || stageSeconds>8 || settleSeconds<0.2 || settleSeconds>=stageSeconds-0.2)
            throw std::invalid_argument("Invalid tonal experiment configuration");
        *this=TonalController();fs_=fs;frequency_=frequency;stageFrames_=uint64_t(stageSeconds*fs);settle_=uint64_t(settleSeconds*fs);ramp_=uint64_t(0.02*fs);tail_=uint64_t(0.1*fs);
    }
    uint64_t sampleCount() const noexcept{return position_;}
    uint64_t totalSamples() const noexcept{return stageFrames_*8;}
    unsigned sampleRate() const noexcept{return fs_;}
    double frequency() const noexcept{return frequency_;}
    int stage() const noexcept{return int(std::min<uint64_t>(position_/stageFrames_,8));}
    bool done() const noexcept{return fault_!=ToneFault::None || position_>=totalSamples();}
    bool complete() const noexcept{return position_>=totalSamples() && fault_==ToneFault::None;}
    bool modelValid() const noexcept{return model_;}
    ToneFault fault() const noexcept{return fault_;}
    void abort(ToneFault f) noexcept{if(fault_==ToneFault::None)fault_=f;}
    const std::array<TonePoint,8>& points() const noexcept{return points_;}
    TonePoint live() const noexcept{return live_;}
    std::complex<double> candidate() const noexcept{return candidate_;}
    double fitError() const noexcept{return fitError_;}
    double predictedTonePower() const noexcept{return predictedTone_;}
    static double dbRatio(double a,double b) noexcept{return 10*std::log10(std::max(a,1e-20)/std::max(b,1e-20));}
    float process(float input) noexcept {
        if(done())return 0;
        if(!std::isfinite(input)){abort(ToneFault::NonFinite);return 0;}
        if(std::abs(input)>=0.95){abort(ToneFault::InputClipping);return 0;}
        int st=stage();uint64_t local=position_%stageFrames_;
        double angle=std::fmod(6.283185307179586*frequency_*double(position_)/fs_,6.283185307179586);
        double c=std::cos(angle),s=std::sin(angle);
        std::complex<double> drive{0,0};
        if(st>=1 && st<=4)drive=std::polar(probeAmplitude(),(st-1)*1.5707963267948966);
        if(st==6 && model_)drive=candidate_;
        double envelope=std::min({1.0,double(local)/std::max<uint64_t>(1,ramp_),double(stageFrames_-1-local)/std::max<uint64_t>(1,ramp_)});
        float output=float(envelope*(drive.real()*c-drive.imag()*s));
        window_.add(input,c,s);if(++windowCount_>=fs_/4){live_=window_.value();window_=ToneMeter();windowCount_=0;
            if(st==6 && local>=settle_ && live_.tone>points_[5].tone*1.5){abort(ToneFault::Amplification);return 0;}}
        if(local>=settle_ && local<stageFrames_-tail_)meter_.add(input,c,s);
        ++position_;
        if(position_%stageFrames_==0){points_[st]=meter_.value();meter_=ToneMeter();
            if(st==0 && (points_[0].tone<1e-9 || points_[0].tone<points_[0].power*0.05))abort(ToneFault::WeakTone);
            if(st==4)fit();
            if((st==5 || st==7) && std::abs(dbRatio(points_[0].tone,points_[st].tone))>1)abort(ToneFault::SourceDrift);
        }
        return fault_==ToneFault::None?output:0;
    }
};
}
