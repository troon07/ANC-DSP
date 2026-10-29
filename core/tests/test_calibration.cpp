#include "anc/calibration.h"
#include "anc/secondary_path.h"
#include "test_support.h"
void test_calibration() {
    anc::FirFilter s; s.configure({0,0,0,0.7f,-0.15f}); std::vector<float> x(10000),y(10000);
    for(size_t k=0;k<x.size();++k){x[k]=noise(); y[k]=s.process(x[k]);}
    auto r=anc::identifySecondaryPath(x,y,24); CHECK(r.validationNmseDb < -60); CHECK(r.peakDelay==3);
    bool threw=false;try{anc::identifySecondaryPath(std::vector<float>(1000),std::vector<float>(1000),24);}catch(...){threw=true;}CHECK(threw);
    x[100]=1.0f;threw=false;try{anc::identifySecondaryPath(x,y,24);}catch(...){threw=true;}CHECK(threw);
    x[100]=0;y[100]=INFINITY;threw=false;try{anc::identifySecondaryPath(x,y,24);}catch(...){threw=true;}CHECK(threw);
}
