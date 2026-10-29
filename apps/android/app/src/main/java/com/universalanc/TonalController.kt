package com.universalanc
import kotlin.math.*

data class TonePoint(val tone:Double=0.0,val power:Double=0.0,val peak:Double=0.0,val count:Long=0)
class ToneMeter {
    private var sx=0.0;private var xx=0.0;private var sc=0.0;private var ss=0.0;private var cc=0.0;private var sn=0.0
    private var cs=0.0;private var xc=0.0;private var xs=0.0;private var peak=0.0;private var n=0L
    fun add(x:Double,c:Double,s:Double){sx+=x;xx+=x*x;sc+=c;ss+=s;cc+=c*c;sn+=s*s;cs+=c*s;xc+=x*c;xs+=x*s;peak=max(peak,abs(x));n++}
    fun value():TonePoint {
        if(n<16)return TonePoint();val a=cc-sc*sc/n;val b=sn-ss*ss/n;val d=cs-sc*ss/n;val u=xc-sx*sc/n;val v=xs-sx*ss/n;val determinant=a*b-d*d
        if(determinant<1e-10)return TonePoint();val real=(u*b-v*d)/determinant;val imag=(v*a-u*d)/determinant
        return TonePoint((real*real+imag*imag)/2,max(0.0,xx/n-(sx/n).pow(2)),peak,n)
    }
}
class TonalController(val rate:Int=48000,val frequency:Double=300.0,val stageSeconds:Double=4.0,val settleSeconds:Double=2.0) {
    init { require(rate in 8000..192000&&frequency.isFinite()&&frequency in 100.0..1000.0&&frequency<rate/4.0&&stageSeconds in 1.0..8.0&&settleSeconds>=0.2&&settleSeconds<stageSeconds-0.2) }
    private val frames=(rate*stageSeconds).toLong();private val settle=(rate*settleSeconds).toLong();private val ramp=(rate*0.02).toLong();private val tail=(rate*0.1).toLong()
    var count=0L;private set
    val total=frames*8
    val stage:Int get()=min(8,(count/frames).toInt())
    var fault="none";private set
    val done:Boolean get()=fault!="none"||count>=total
    val complete:Boolean get()=fault=="none"&&count>=total
    var modelValid=false;private set
    var real=0.0;private set
    var imag=0.0;private set
    var fitError=0.0;private set
    var predictedTone=0.0;private set
    val points=Array(8){TonePoint()}
    private var meter=ToneMeter();private var window=ToneMeter();private var windowCount=0
    var live=TonePoint();private set
    fun abort(reason:String){if(fault=="none")fault=reason}
    fun process(input:Float):Float {
        if(done)return 0f
        if(!input.isFinite()){abort("nonfinite microphone sample");return 0f}
        if(abs(input)>=0.95f){abort("microphone clipping");return 0f}
        val st=stage;val local=count%frames;val angle=(2*PI*frequency*count/rate)%(2*PI);val c=cos(angle);val s=sin(angle)
        var re=0.0;var im=0.0
        if(st in 1..4){re=0.02*cos((st-1)*PI/2);im=0.02*sin((st-1)*PI/2)}
        if(st==6&&modelValid){re=real;im=imag}
        val envelope=minOf(1.0,local.toDouble()/max(1L,ramp),(frames-1-local).toDouble()/max(1L,ramp));val output=(envelope*(re*c-im*s)).toFloat()
        window.add(input.toDouble(),c,s);windowCount++
        if(windowCount>=rate/4){live=window.value();window=ToneMeter();windowCount=0
            if(st==6&&local>=settle&&live.tone>points[5].tone*1.5){abort("anti-tone increased measured target power");return 0f}}
        if(local>=settle&&local<frames-tail)meter.add(input.toDouble(),c,s)
        count++
        if(count%frames==0L){points[st]=meter.value();meter=ToneMeter()
            if(st==0&&(points[0].tone<1e-9||points[0].tone<points[0].power*0.05))abort("target tone missing or too weak")
            if(st==4)fit()
            if((st==5||st==7)&&abs(db(points[0].tone,points[st].tone))>1)abort("ANC-off tone level changed by more than 1 dB")
        }
        return if(fault=="none")output else 0f
    }
    private fun fit(){
        val base=points[0].tone;val a=0.02;val p0=points[1].tone;val p1=points[2].tone;val p2=points[3].tone;val p3=points[4].tone;val mean=(p0+p1+p2+p3)/4
        fitError=abs(p0+p2-p1-p3)/(4*max(mean,1e-20))
        if(base<1e-9||base<max(1e-12,points[0].power)*0.05){abort("target tone missing or too weak");return}
        if(fitError>0.1){abort("source phase/gain changed during calibration");return}
        val b=(mean-base)/(a*a)
        if(b<=max(1e-12,base*1e-4/(a*a))){abort("earbud response cannot be identified");return}
        val gr=(p0-p2)/(4*a);val gi=(p3-p1)/(4*a);real=-gr/b;imag=gi/b
        predictedTone=base-(gr*gr+gi*gi)/b
        if(predictedTone< -0.05*base||predictedTone>0.5*base){abort("source phase/gain changed during calibration");return}
        predictedTone=max(0.0,predictedTone)
        if(!hypot(real,imag).isFinite()||hypot(real,imag)>0.08){abort("required output exceeds 0.08 FS cap; move earbud closer");return}
        modelValid=true
    }
    companion object {
        fun db(a:Double,b:Double)=10*log10(max(a,1e-20)/max(b,1e-20))
        fun stageName(stage:Int)=arrayOf("Baseline OFF","Probe 0 degrees","Probe 90 degrees","Probe 180 degrees","Probe 270 degrees","Validation OFF","Candidate ON","Recheck OFF","Finished")[stage.coerceIn(0,8)]
    }
}
