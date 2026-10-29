#pragma once
#include "filter.h"
#include <limits>
namespace anc {
enum class AdaptiveAlgorithm { Lms, Nlms, Rls };
struct AdaptiveConfig {
    AdaptiveAlgorithm algorithm=AdaptiveAlgorithm::Nlms;
    size_t taps=32;
    float step=0.15f,leakage=0,epsilon=1e-8f;
    double forgetting=0.995,initialInverseCovariance=10;
    float minimumReferencePower=1e-10f,maximumCoefficientNorm=20;
};
struct AdaptiveSample { float estimate=0,residual=0;bool adapted=false,valid=true; };
class AdaptiveFir {
    AdaptiveConfig config_;
    std::array<float,MaxTaps> w_{},x_{};
    std::array<double,128> u_{};
    std::vector<double> covariance_; // Allocated only by configure, never by process.
    size_t position_=0;bool configured_=false,fault_=false;
public:
    void configure(const AdaptiveConfig& c){
        if(!c.taps||c.taps>MaxTaps||!std::isfinite(c.step)||c.step<=0||!std::isfinite(c.leakage)||c.leakage<0||c.leakage>=1||!std::isfinite(c.epsilon)||c.epsilon<=0||!std::isfinite(c.minimumReferencePower)||c.minimumReferencePower<0||!std::isfinite(c.maximumCoefficientNorm)||c.maximumCoefficientNorm<=0)
            throw std::invalid_argument("Invalid adaptive filter configuration");
        if(c.algorithm==AdaptiveAlgorithm::Nlms&&c.step>=2)throw std::invalid_argument("NLMS step must be below 2");
        if(c.algorithm==AdaptiveAlgorithm::Rls&&(c.taps>128||!std::isfinite(c.forgetting)||c.forgetting<=0.9||c.forgetting>1||!std::isfinite(c.initialInverseCovariance)||c.initialInverseCovariance<=0||c.leakage!=0))
            throw std::invalid_argument("RLS requires <=128 taps, 0.9<lambda<=1, positive covariance and no LMS leakage");
        if(c.algorithm!=AdaptiveAlgorithm::Lms&&c.algorithm!=AdaptiveAlgorithm::Nlms&&c.algorithm!=AdaptiveAlgorithm::Rls)throw std::invalid_argument("Unknown adaptive algorithm");
        configured_=false;config_=c;covariance_.assign(c.algorithm==AdaptiveAlgorithm::Rls?c.taps*c.taps:0,0);configured_=true;reset();
    }
    void reset() noexcept {
        w_.fill(0);x_.fill(0);position_=0;fault_=false;
        std::fill(covariance_.begin(),covariance_.end(),0.0);
        if(configured_&&config_.algorithm==AdaptiveAlgorithm::Rls)for(size_t k=0;k<config_.taps;++k)covariance_[k*config_.taps+k]=config_.initialInverseCovariance;
    }
    bool faulted()const noexcept{return fault_;}
    double coefficientNorm()const noexcept{double e=0;for(size_t k=0;k<config_.taps;++k)e+=double(w_[k])*w_[k];return std::sqrt(e);}
    std::vector<float> weights()const{return {w_.begin(),w_.begin()+config_.taps};}
    AdaptiveSample process(float reference,float primary,bool adapt=true) noexcept {
        if(!configured_||fault_)return {0,0,false,false};
        if(!std::isfinite(reference)||!std::isfinite(primary)){fault_=true;return {0,0,false,false};}
        const size_t n=config_.taps;x_[position_]=reference;
        auto value=[&](size_t k){return x_[(position_+n-k)%n];};
        double estimate=0,power=0;for(size_t k=0;k<n;++k){double v=value(k);estimate+=w_[k]*v;power+=v*v;}
        double error=double(primary)-estimate;bool enabled=adapt&&power/n>=config_.minimumReferencePower;
        if(enabled&&config_.algorithm==AdaptiveAlgorithm::Rls){
            for(size_t i=0;i<n;++i){double sum=0;for(size_t j=0;j<n;++j)sum+=covariance_[i*n+j]*value(j);u_[i]=sum;}
            double denom=config_.forgetting;for(size_t k=0;k<n;++k)denom+=value(k)*u_[k];
            if(!std::isfinite(denom)||denom<=config_.epsilon){fault_=true;return {0,0,false,false};}
            for(size_t k=0;k<n;++k)w_[k]+=float(u_[k]*error/denom);
            for(size_t i=0;i<n;++i)for(size_t j=i;j<n;++j){double p=(covariance_[i*n+j]-u_[i]*u_[j]/denom)/config_.forgetting;covariance_[i*n+j]=covariance_[j*n+i]=p;if(!std::isfinite(p)||(i==j&&p<=0))fault_=true;}
        }else if(enabled){
            double gain=config_.step*error;
            if(config_.algorithm==AdaptiveAlgorithm::Nlms)gain/=config_.epsilon+power;
            for(size_t k=0;k<n;++k)w_[k]=float((1-config_.leakage)*w_[k]+gain*value(k));
        }
        double norm=coefficientNorm();
        if(!std::isfinite(norm)||!std::isfinite(error)||std::abs(estimate)>std::numeric_limits<float>::max()||std::abs(error)>std::numeric_limits<float>::max())fault_=true;
        if(norm>config_.maximumCoefficientNorm&&std::isfinite(norm)){float gain=float(config_.maximumCoefficientNorm/norm);for(size_t k=0;k<n;++k)w_[k]*=gain;}
        position_=(position_+1)%n;
        if(fault_)return {0,0,false,false};
        return {float(estimate),float(error),enabled,true};
    }
    bool processBlock(const float* reference,const float* primary,float* residual,float* estimate,size_t frames,bool adapt=true) noexcept {
        if(frames&&(!reference||!primary||!residual)){fault_=true;if(residual)std::fill(residual,residual+frames,0);if(estimate)std::fill(estimate,estimate+frames,0);return false;}
        for(size_t k=0;k<frames;++k){auto v=process(reference[k],primary[k],adapt);residual[k]=v.residual;if(estimate)estimate[k]=v.estimate;}
        if(fault_&&frames){std::fill(residual,residual+frames,0);if(estimate)std::fill(estimate,estimate+frames,0);}
        return configured_&&!fault_;
    }
};
struct ConditioningConfig { float sampleRate=8000,dcCutoff=10,lowpass=1500;bool enabled=true; };
class SignalConditioner {
    DcBlocker dc_;Biquad low_;bool enabled_=false;
public:
    void configure(const ConditioningConfig& c){
        if(!std::isfinite(c.sampleRate)||c.sampleRate<8000||!std::isfinite(c.dcCutoff)||c.dcCutoff<=0||!std::isfinite(c.lowpass)||c.lowpass<=c.dcCutoff||c.lowpass>=c.sampleRate/2)throw std::invalid_argument("Invalid conditioning passband");
        dc_.configure(c.sampleRate,c.dcCutoff);low_.lowpass(c.sampleRate,c.lowpass);enabled_=c.enabled;
    }
    float process(float x)noexcept{return enabled_?low_.process(dc_.process(x)):x;}
    void reset()noexcept{dc_.reset();low_.reset();}
};
class AdaptiveNoiseCanceller {
    AdaptiveFir filter_;SignalConditioner reference_,primary_;bool configured_=false;
public:
    void configure(const AdaptiveConfig& a,const ConditioningConfig& c){configured_=false;filter_.configure(a);reference_.configure(c);primary_.configure(c);configured_=true;}
    AdaptiveSample process(float reference,float primary,bool adapt=true)noexcept{
        if(!configured_)return {0,0,false,false};
        if(!std::isfinite(reference)||!std::isfinite(primary))return filter_.process(reference,primary,adapt);
        return filter_.process(reference_.process(reference),primary_.process(primary),adapt);
    }
    bool processBlock(const float*reference,const float*primary,float*residual,float*estimate,size_t frames,bool adapt=true)noexcept{
        if(frames&&(!reference||!primary||!residual))return filter_.processBlock(reference,primary,residual,estimate,frames,adapt);
        bool valid=true;for(size_t k=0;k<frames;++k){auto v=process(reference[k],primary[k],adapt);residual[k]=v.residual;if(estimate)estimate[k]=v.estimate;valid=valid&&v.valid;}
        if((!valid||!configured_)&&frames){std::fill(residual,residual+frames,0);if(estimate)std::fill(estimate,estimate+frames,0);}return valid&&configured_;
    }
    const AdaptiveFir& filter()const noexcept{return filter_;}
};
}
