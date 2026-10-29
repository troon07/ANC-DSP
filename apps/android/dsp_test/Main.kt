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
    println("PASS Kotlin tonal loop with 100 ms modeled delay and silence rejection")
    println("PASS Kotlin FxNLMS: low_delay=$causal dB, delay40=$wired dB, delay400=$wireless dB; NaN gate")
}
