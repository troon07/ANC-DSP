#include "anc/metrics.h"
#include "test_support.h"
void test_metrics(){ anc::SignalMetrics m; m.add(1);m.add(-1);CHECK(m.rms()==1);CHECK(m.peak==1);CHECK(std::abs(anc::attenuationDb(100,1)-20)<1e-8); }
