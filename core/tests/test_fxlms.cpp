#include "anc/dsp_engine.h"
#include "anc/metrics.h"
#include "test_support.h"
#include <limits>
double run(size_t primaryDelay,size_t secondaryDelay) {
    std::vector<float> p(primaryDelay+1,0),s(secondaryDelay+1,0); p.back()=0.7f;s.back()=0.8f;
    anc::FirFilter primary,secondary; primary.configure(p);secondary.configure(s);
    anc::FxLms f; f.configure(64,s,0.02f); double de=0,ee=0;
    for(int i=0;i<80000;++i) {float x=noise(),d=primary.process(x),y=f.output(x),e=d+secondary.process(y); f.adapt(e);if(i>60000){de+=d*d;ee+=e*e;} CHECK(f.finite());}
    return anc::attenuationDb(de,ee);
}
void test_fxlms() {
    CHECK(run(24,8)>25); CHECK(run(8,40)<3);
    anc::DspEngine engine; CHECK(engine.process(1,1)==0);
    anc::SecondaryPath s{8000,"simulation",{0,1}}; anc::DspConfig c; bool threw=false;
    try {engine.configure(c,s,"simulation",true);}catch(...){threw=true;} CHECK(threw);
    s.route="fixture"; engine.configure(c,s,"fixture",true);
    CHECK(engine.process(std::numeric_limits<float>::quiet_NaN(),0)==0); CHECK(engine.process(1,1)==0);
    threw=false; try{engine.configure(c,s,"other-route",true);}catch(...){threw=true;}CHECK(threw);
}
