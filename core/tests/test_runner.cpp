#include <iostream>
#include <exception>
void test_tonal(); void test_filter(); void test_lms(); void test_fxlms(); void test_calibration(); void test_metrics();
bool test_stability_nan_trip(); bool test_stability_clipping_trip(); bool test_soft_clip();
int main() {
    int failures=0;
    const char* names[]={"filters","NLMS identification","FxNLMS causality and engine gates","calibration","metrics","tonal physical-loop model and rejection gates"};
    void (*tests[])()={test_filter,test_lms,test_fxlms,test_calibration,test_metrics,test_tonal};
    for(int i=0;i<6;++i) { try { tests[i](); std::cout<<"PASS "<<names[i]<<'\n'; } catch(const std::exception& e) {++failures;std::cerr<<"FAIL "<<names[i]<<": "<<e.what()<<'\n';} }
    if(!test_stability_nan_trip() || !test_stability_clipping_trip() || !test_soft_clip()) ++failures;
    else std::cout<<"PASS safety: nonfinite, clipping, bounded output\n";
    return failures?1:0;
}
