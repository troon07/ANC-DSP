#include "anc/adaptive.h"
#include "anc/dsp_engine.h"
#include "test_support.h"
#include <limits>
void test_adaptive(){
    for(auto mode:{anc::AdaptiveAlgorithm::Lms,anc::AdaptiveAlgorithm::Nlms,anc::AdaptiveAlgorithm::Rls}){
        anc::AdaptiveFir f;anc::AdaptiveConfig c;c.algorithm=mode;c.taps=8;c.step=mode==anc::AdaptiveAlgorithm::Lms?0.4f:0.2f;c.forgetting=0.999;f.configure(c);anc::FirFilter plant;plant.configure({0.7f,-0.2f,0,0.1f});
        double error=0;for(int k=0;k<24000;++k){float x=noise();auto v=f.process(x,plant.process(x));CHECK(v.valid);if(k>20000)error+=v.residual*v.residual;}
        CHECK(error<1e-5);CHECK(std::abs(f.weights()[0]-0.7f)<0.002);auto before=f.weights();for(int k=0;k<100;++k)f.process(noise(),noise(),false);CHECK(before==f.weights());
        CHECK(!f.process(std::numeric_limits<float>::infinity(),0).valid);CHECK(f.faulted());f.reset();CHECK(f.process(0,0).valid);
    }
    // Block output must agree with the sample API across non-power-of-two chunks.
    anc::AdaptiveNoiseCanceller a,b;anc::AdaptiveConfig c;c.taps=16;a.configure(c,{});b.configure(c,{});
    std::array<float,257>x{},p{},sample{},block{},estimate{};for(size_t k=0;k<x.size();++k){x[k]=noise();p[k]=0.8f*x[k]+noise()*0.1f;sample[k]=a.process(x[k],p[k]).residual;}
    CHECK(b.processBlock(x.data(),p.data(),block.data(),estimate.data(),73));CHECK(b.processBlock(x.data()+73,p.data()+73,block.data()+73,estimate.data()+73,184));
    for(size_t k=0;k<x.size();++k)CHECK(sample[k]==block[k]);
    p[100]=std::numeric_limits<float>::quiet_NaN();CHECK(!b.processBlock(x.data(),p.data(),block.data(),nullptr,x.size()));for(float v:block)CHECK(v==0);
    bool threw=false;c.algorithm=anc::AdaptiveAlgorithm::Rls;c.taps=129;try{anc::AdaptiveFir r;r.configure(c);}catch(...){threw=true;}CHECK(threw);
    anc::AdaptiveNoiseCanceller unconfigured;CHECK(!unconfigured.process(0,0).valid);
    anc::AdaptiveConfig bounded;bounded.taps=1;bounded.algorithm=anc::AdaptiveAlgorithm::Lms;bounded.maximumCoefficientNorm=2;anc::AdaptiveFir projected;projected.configure(bounded);projected.process(1,1000);CHECK(projected.coefficientNorm()<=2.00001);
    // Acoustic block faults mute the complete block, preserving the trip reason.
    anc::DspEngine live;anc::DspConfig cfg;anc::SecondaryPath s{8000,"fixture-test",{0,0.8f}};live.configure(cfg,s,s.route,true);
    CHECK(!live.processBlock(x.data(),p.data(),block.data(),block.size()));for(float v:block)CHECK(v==0);CHECK(live.tripReason()==anc::TripReason::NonFiniteSample);
    live.configure(cfg,s,s.route,true);for(int i=0;i<1000;++i)live.process(0,0);CHECK(live.telemetry().adapted==0);CHECK(live.telemetry().frozen==1000);
    cfg.controllerTaps=1;cfg.stepSize=0.005f;anc::SecondaryPath tiny{8000,"fixture-test",{0.0001f}};live.configure(cfg,tiny,tiny.route,true);
    for(int k=0;k<2000&&live.isArmed();++k)live.process(0.1f,0.1f);
    CHECK(!live.isArmed());CHECK(live.tripReason()==anc::TripReason::SustainedOutputLimit);
    // Watchdog uses an explicit matched ANC-off error baseline, not reference RMS.
    cfg.baselineErrorRms=0.001f;live.configure(cfg,s,s.route,true);for(int i=0;i<16000&&live.isArmed();++i)live.process(0,0.1f);CHECK(!live.isArmed());CHECK(live.tripReason()==anc::TripReason::ErrorGrowth);
}
