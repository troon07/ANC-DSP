#include "anc/lms.h"
#include "test_support.h"
void test_lms() {
    anc::Nlms n; n.configure(16,0.3f); anc::FirFilter s; s.configure({0,0,0.6f,-0.2f,0.1f});
    float e=0; for(int i=0;i<12000;++i) {float x=noise();e=n.process(x,s.process(x));} CHECK(std::abs(e)<1e-5);
    CHECK(std::abs(n.weights()[2]-0.6f)<1e-4);
}
