package com.universalanc
import kotlin.math.abs
import kotlin.math.log10
import kotlin.math.sin
import kotlin.math.max
import java.util.Locale

// Kotlin port for Android: preallocated arrays; no per-sample allocation.
class FxController(private val secondary: FloatArray, private val length: Int=64) {
    private val w=FloatArray(length);private val x=FloatArray(length);private val fx=FloatArray(length)
    private val sx=FloatArray(secondary.size); private var pos=0;private var sp=0
    var tripped=false;private set
    fun output(reference: Float): Float {
        if(!reference.isFinite()){tripped=true;return 0f}
        x[pos]=reference;sx[sp]=reference;var filtered=0f;var j=sp
        for(k in secondary.indices){filtered+=secondary[k]*sx[j];j=if(j==0)sx.lastIndex else j-1}
        sp=(sp+1)%sx.size;fx[pos]=filtered;var y=0f;j=pos
        for(k in 0 until length){y+=w[k]*x[j];j=if(j==0)length-1 else j-1}
        if(!y.isFinite()){tripped=true;return 0f};return y
    }
    fun adapt(error: Float,enabled: Boolean=true){
        if(!error.isFinite()){tripped=true;return}
        if(enabled&&!tripped){var power=1e-8f;for(v in fx)power+=v*v;val gain=0.02f*error/power;var j=pos
            for(k in 0 until length){w[k]=(1-1e-6f)*w[k]-gain*fx[j];if(!w[k].isFinite())tripped=true;j=if(j==0)length-1 else j-1}}
        pos=(pos+1)%length
    }
}
object AncSimulation {
    fun run(delay: Int=8): Double {
        val s=FloatArray(delay+3);s[delay]=0.8f;s[delay+1]=-0.15f;s[delay+2]=0.05f
        val f=FxController(s);val primary=FloatArray(27);val sy=FloatArray(s.size)
        var pp=0;var sp=0;var state=42L;var de=0.0;var ee=0.0
        for(n in 0 until 48000){
            state=(1664525*state+1013904223) and 0xffffffffL;val x=((state/4294967296.0*2-1)*0.08).toFloat()
            state=(1664525*state+1013904223) and 0xffffffffL
            primary[pp]=x;fun past(d:Int)=primary[(pp-d+primary.size)%primary.size]
            val d=0.65f*past(24)+0.15f*past(25)-0.05f*past(26);pp=(pp+1)%primary.size
            val raw=f.output(x);val y=raw.coerceIn(-0.2f,0.2f);sy[sp]=y;var control=0f;var j=sp
            for(k in s.indices){control+=s[k]*sy[j];j=if(j==0)sy.lastIndex else j-1};sp=(sp+1)%s.size
            val e=d+control;f.adapt(e,abs(raw)<0.2f);check(!f.tripped)
            if(n>=36000){de+=d*d;ee+=e*e}
        }
        return 10*log10(max(de,1e-20)/max(ee,1e-20))
    }
    fun report(): String {
        val a=run();val b=run(40);val c=run(400)
        check(a>20&&b<3&&c<3)
        return String.format(Locale.US,"SIMULATION · 8 kHz, 6 s, 64 taps\nLow-delay model: %.2f dB\n5 ms secondary delay: %.2f dB\n50 ms secondary delay: %.2f dB\n\nDeterministic broadband test. Physical attenuation has not been measured.",a,b,c)
    }
}
