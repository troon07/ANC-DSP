import com.universalanc.AncSimulation
import com.universalanc.FxController
fun main(){
    val causal=AncSimulation.run();val wired=AncSimulation.run(40);val wireless=AncSimulation.run(400)
    check(causal>20&&wired<3&&wireless<3)
    check(kotlin.math.abs(causal-48.21)<0.1)
    val f=FxController(floatArrayOf(0f,0.8f));check(f.output(Float.NaN)==0f&&f.tripped)
    val tonal=com.universalanc.TonalController(8000,300.0,1.0,0.25);val delay=FloatArray(800);var pos=0
    for(n in 0 until 64000){if(tonal.done)break;val input=(0.03*kotlin.math.cos(2*Math.PI*300*n/8000+0.6)+0.7*delay[pos]).toFloat();delay[pos]=tonal.process(input);pos=(pos+1)%delay.size}
    check(tonal.complete&&tonal.modelValid);check(tonal.points[6].tone<tonal.points[5].tone*0.01)
    val silent=com.universalanc.TonalController(8000,300.0,1.0,0.25);repeat(8000){silent.process(0f)};check(silent.fault=="target tone missing or too weak")
    for(method in com.universalanc.AdaptiveMethod.values()){
        val digital=com.universalanc.AdaptiveDsp(method,8,8000,if(method==com.universalanc.AdaptiveMethod.LMS)0.4 else 0.2,0.999,false)
        var state=73L;val history=FloatArray(4);var error=0.0
        for(n in 0 until 24000){state=(1664525*state+1013904223) and 0xffffffffL;val x=((state/4294967296.0*2-1)*0.2).toFloat();for(k in 3 downTo 1)history[k]=history[k-1];history[0]=x;val desired=0.7f*history[0]-0.2f*history[1]+0.1f*history[3];val e=digital.process(x,desired);check(!digital.faulted);if(n>20000)error+=e*e}
        check(error<1e-5);check(digital.process(Float.NaN,0f)==0f&&digital.faulted)
    }
    val traceDirectory=java.io.File(System.getProperty("anc.traceDirectory",""))
    if(traceDirectory.isDirectory){
        var maxDelta=0.0;var cases=0
        for(scenario in listOf("stationary","path_change","uncorrelated","reference_leakage","reference_dropout"))for(method in com.universalanc.AdaptiveMethod.values()){
            val file=java.io.File(traceDirectory,"${scenario}_${method.name.lowercase()}.csv");if(!file.exists())continue
            val dsp=com.universalanc.AdaptiveDsp(method,16,8000,if(method==com.universalanc.AdaptiveMethod.LMS)0.1 else 0.05,0.999,true)
            file.bufferedReader().use{reader->reader.readLine();reader.forEachLine{line->val fields=line.split(',');val y=dsp.process(fields[1].toFloat(),fields[2].toFloat());check(!dsp.faulted);maxDelta=maxOf(maxDelta,kotlin.math.abs(y-fields[7].toDouble()))}}
            cases++
        }
        check(cases==15&&maxDelta<2e-4);println("PASS Kotlin/C++ full DSP sample agreement: $cases cases; max delta $maxDelta FS")
    }
    println("PASS Kotlin digital LMS/NLMS/RLS convergence and nonfinite gate")
    println("PASS Kotlin tonal loop with 100 ms modeled delay and silence rejection")
    println("PASS Kotlin FxNLMS: low_delay=$causal dB, delay40=$wired dB, delay400=$wireless dB; NaN gate")
}
