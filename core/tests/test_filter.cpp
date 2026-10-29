#include "anc/filter.h"
#include "test_support.h"
void test_filter() {
    anc::FirFilter f; f.configure({1,2,3}); CHECK(f.process(1)==1); CHECK(f.process(0)==2); CHECK(f.process(0)==3); CHECK(f.process(0)==0);
    anc::DcBlocker d; d.configure(8000); float y=1; for(int i=0;i<20000;++i)y=d.process(1); CHECK(std::abs(y)<1e-5);
    anc::Biquad b; b.lowpass(8000,700); for(int i=0;i<20000;++i)y=b.process(1); CHECK(std::abs(y-1)<1e-4);
    bool threw=false; try { f.configure({}); } catch(...) {threw=true;} CHECK(threw);
}
