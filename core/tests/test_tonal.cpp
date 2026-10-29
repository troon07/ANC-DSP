#include "anc/tonal.h"
#include "test_support.h"
#include <limits>
void test_tonal(){
    anc::TonalController f;f.configure(8000,300,1,0.25);
    // Synthetic acoustic loop with a 100 ms output delay and a stationary source.
    std::array<float,800> delay{};size_t pos=0;
    for(size_t n=0;n<64000 && !f.done();++n){
        double phase=6.283185307179586*300*n/8000;
        float mic=float(0.03*std::cos(phase+0.6)+0.7*delay[pos]);
        float y=f.process(mic);CHECK(std::abs(y)<=0.08001);delay[pos]=y;pos=(pos+1)%delay.size();
    }
    CHECK(f.complete());CHECK(f.modelValid());CHECK(f.points()[6].tone<f.points()[5].tone*0.01);
    anc::TonalController silent;silent.configure(8000,300,1,0.25);
    for(int i=0;i<8000;++i)silent.process(0);CHECK(silent.fault()==anc::ToneFault::WeakTone);
    anc::TonalController invalid;invalid.configure(8000,300);CHECK(invalid.process(std::numeric_limits<float>::quiet_NaN())==0);CHECK(invalid.done());
    // Real changes of disturbance level must not be reported as cancellation.
    anc::TonalController drift;drift.configure(8000,300,1,0.25);delay.fill(0);pos=0;
    for(size_t n=0;n<64000 && !drift.done();++n){double a=n>=40000?0.06:0.03;
        float x=float(a*std::cos(6.283185307179586*300*n/8000+0.6)+0.7*delay[pos]);float y=drift.process(x);delay[pos]=y;pos=(pos+1)%delay.size();}
    CHECK(drift.fault()==anc::ToneFault::SourceDrift);
}
