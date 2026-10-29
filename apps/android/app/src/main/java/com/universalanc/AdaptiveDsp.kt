package com.universalanc
import kotlin.math.*

enum class AdaptiveMethod { LMS, NLMS, RLS }
// Streaming reference-based digital noise cancellation. This is NOT anti-noise playback.
// Constructor allocates state; process/processBlock allocate no per-sample objects.
class AdaptiveDsp(val method:AdaptiveMethod=AdaptiveMethod.NLMS,val taps:Int=32,val rate:Int=8000,
                  val step:Double=0.15,val forgetting:Double=0.995,val conditioning:Boolean=true) {
    init { require(taps in 1..4096&&rate in 8000..192000&&step.isFinite()&&step>0)
        require(method!=AdaptiveMethod.NLMS||step<2)
        require(method!=AdaptiveMethod.RLS||(taps<=128&&forgetting.isFinite()&&forgetting>0.9&&forgetting<=1)) }
    private val weights=DoubleArray(taps);private val history=DoubleArray(taps)
    private val covariance=DoubleArray(if(method==AdaptiveMethod.RLS)taps*taps else 0)
    private val u=DoubleArray(if(method==AdaptiveMethod.RLS)taps else 0)
    private var position=0
    private val referenceFilter=Conditioner(rate);private val primaryFilter=Conditioner(rate)
    var estimate=0f;private set
    var faulted=false;private set
    var adapted=false;private set
    init {reset()}
    fun reset(){weights.fill(0.0);history.fill(0.0);covariance.fill(0.0);if(method==AdaptiveMethod.RLS)for(k in 0 until taps)covariance[k*taps+k]=10.0
        referenceFilter.reset();primaryFilter.reset();position=0;estimate=0f;faulted=false;adapted=false}
    fun coefficientNorm():Double {var p=0.0;for(v in weights)p+=v*v;return sqrt(p)}
    private fun value(k:Int)=history[(position+taps-k)%taps]
    fun process(reference:Float,primary:Float,adapt:Boolean=true):Float {
        adapted=false
        if(faulted)return 0f
        if(!reference.isFinite()||!primary.isFinite()){faulted=true;estimate=0f;return 0f}
        val x=if(conditioning)referenceFilter.process(reference.toDouble()) else reference.toDouble()
        val d=if(conditioning)primaryFilter.process(primary.toDouble()) else primary.toDouble()
        history[position]=x;var y=0.0;var power=0.0
        for(k in 0 until taps){val v=value(k);y+=weights[k]*v;power+=v*v}
        val error=d-y;adapted=adapt&&power/taps>=1e-10
        if(adapted&&method==AdaptiveMethod.RLS){
            for(i in 0 until taps){var sum=0.0;for(j in 0 until taps)sum+=covariance[i*taps+j]*value(j);u[i]=sum}
            var denominator=forgetting;for(k in 0 until taps)denominator+=value(k)*u[k]
            if(!denominator.isFinite()||denominator<=1e-8){faulted=true;estimate=0f;return 0f}
            for(k in 0 until taps)weights[k]+=u[k]*error/denominator
            for(i in 0 until taps)for(j in i until taps){val p=(covariance[i*taps+j]-u[i]*u[j]/denominator)/forgetting;covariance[i*taps+j]=p;covariance[j*taps+i]=p;if(!p.isFinite()||(i==j&&p<=0))faulted=true}
        }else if(adapted){val scale=step*error/(if(method==AdaptiveMethod.NLMS)1e-8+power else 1.0);for(k in 0 until taps)weights[k]+=scale*value(k)}
        val norm=coefficientNorm();if(!norm.isFinite()||!error.isFinite()||abs(error)>Float.MAX_VALUE||abs(y)>Float.MAX_VALUE)faulted=true
        if(norm>20&&norm.isFinite()){val scale=20/norm;for(k in 0 until taps)weights[k]*=scale}
        position=(position+1)%taps
        if(faulted){estimate=0f;return 0f};estimate=y.toFloat();return error.toFloat()
    }
    fun processBlock(reference:FloatArray,primary:FloatArray,residual:FloatArray,count:Int=reference.size,adapt:Boolean=true):Boolean {
        require(count>=0&&count<=reference.size&&count<=primary.size&&count<=residual.size)
        for(k in 0 until count)residual[k]=process(reference[k],primary[k],adapt)
        if(faulted)java.util.Arrays.fill(residual,0,count,0f)
        return !faulted
    }
    private class Conditioner(rate:Int) {
        private val r=exp(-2*PI*10/rate);private val w=2*PI*1500/rate;private val c=cos(w);private val alpha=sin(w)/sqrt(2.0)
        private val b0=(1-c)/(2*(1+alpha));private val b1=2*b0;private val a1=-2*c/(1+alpha);private val a2=(1-alpha)/(1+alpha)
        private var previous=0.0;private var dc=0.0;private var z1=0.0;private var z2=0.0
        fun reset(){previous=0.0;dc=0.0;z1=0.0;z2=0.0}
        fun process(x:Double):Double {val hp=x-previous+r*dc;previous=x;dc=hp;val y=b0*hp+z1;z1=b1*hp-a1*y+z2;z2=b0*hp-a2*y;return y}
    }
}
