package com.universalanc
import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.media.*
import android.os.Process
import java.io.File
import kotlin.math.abs

data class AncProfile(val rate: Int,val route: String,val taps: FloatArray) {
    companion object {
        fun parse(text: String): AncProfile {
            val t=text.trim().split(Regex("\\s+"));require(t.size>=5&&t[0]=="ANC_PROFILE_V1") {"Invalid profile header"}
            val n=t[3].toInt();require(n in 1..4096&&t.size==n+4);val taps=FloatArray(n){t[it+4].toFloat()}
            require(taps.all{it.isFinite()}&&taps.any{abs(it)>1e-6f});return AncProfile(t[1].toInt(),t[2],taps)
        }
    }
}
class AncAudioEngine(private val context: Context) {
    @Volatile private var running=false
    private var thread: Thread?=null
    private var recorder: AudioRecord?=null
    private val manager=context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
    private val routeCallback=object: AudioDeviceCallback(){ override fun onAudioDevicesRemoved(removedDevices: Array<out AudioDeviceInfo>){running=false;try{recorder?.stop()}catch(_:Exception){}} }
    @SuppressLint("MissingPermission")
    fun start(input: AudioDeviceInfo,output: AudioDeviceInfo,profile: AncProfile?,recordProbe: Boolean,onComplete:(String)->Unit){
        require(thread==null){"Stop the existing stream first"}
        val helper=AudioDeviceHelper(context);require(helper.fixture(input,output)){"Select stereo USB fixture input and USB output"}
        val rate=48000;val token=helper.token(input,output)
        if(!recordProbe)require(profile!=null&&profile.route==token&&profile.rate==rate&&profile.route!="simulation"){"Load a 48 kHz calibration for this exact Android route"}
        running=true;manager.registerAudioDeviceCallback(routeCallback,null)
        thread=Thread {
            Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO)
            var record: AudioRecord?=null;var track: AudioTrack?=null;var status="Stopped";var count=0
            val maxFrames=rate*10;val probe=FloatArray(maxFrames);val response=FloatArray(maxFrames)
            try {
                val minIn=AudioRecord.getMinBufferSize(rate,AudioFormat.CHANNEL_IN_STEREO,AudioFormat.ENCODING_PCM_FLOAT)
                val minOut=AudioTrack.getMinBufferSize(rate,AudioFormat.CHANNEL_OUT_STEREO,AudioFormat.ENCODING_PCM_FLOAT)
                require(minIn>0&&minOut>0){"Stereo float audio unsupported"}
                record=AudioRecord.Builder().setAudioSource(MediaRecorder.AudioSource.UNPROCESSED).setAudioFormat(AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_FLOAT).setSampleRate(rate).setChannelMask(AudioFormat.CHANNEL_IN_STEREO).build()).setBufferSizeInBytes(maxOf(minIn,4096)).build()
                track=AudioTrack.Builder().setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build()).setAudioFormat(AudioFormat.Builder().setEncoding(AudioFormat.ENCODING_PCM_FLOAT).setSampleRate(rate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).build()).setBufferSizeInBytes(maxOf(minOut,4096)).setTransferMode(AudioTrack.MODE_STREAM).setPerformanceMode(AudioTrack.PERFORMANCE_MODE_LOW_LATENCY).build()
                require(record.state==AudioRecord.STATE_INITIALIZED&&track.state==AudioTrack.STATE_INITIALIZED)
                require(record.setPreferredDevice(input)&&track.setPreferredDevice(output)){"Route request rejected"}
                val buffer=FloatArray(256);val out=FloatArray(256);val controller=profile?.let{FxController(it.taps,256)};var random=42L;var clipped=0
                recorder=record;record.startRecording();track.play()
                val end=System.nanoTime()+10_000_000_000L
                while(running&&count<maxFrames&&System.nanoTime()<end){
                    val n=record.read(buffer,0,buffer.size,AudioRecord.READ_BLOCKING);require(n>0&&n%2==0){"Capture stopped ($n)"}
                    require(record.routedDevice?.id==input.id&&track.routedDevice?.id==output.id){"Actual route changed or preferred route was not honored"}
                    for(k in 0 until n/2){val ref=buffer[2*k];val error=buffer[2*k+1];require(ref.isFinite()&&error.isFinite())
                        var y=0f
                        if(count<maxFrames){
                            if(recordProbe){random=(1664525*random+1013904223) and 0xffffffffL;y=((random/4294967296.0*2-1)*0.02).toFloat();probe[count]=y;response[count]=error}
                            else if(controller!=null){val raw=controller.output(ref);if(abs(raw)>=0.95f)clipped++ else clipped=0;require(clipped<16&&!controller.tripped){"Controller safety trip"};y=raw.coerceIn(-0.2f,0.2f);controller.adapt(error,abs(raw)<0.2f)}
                            count++
                        };out[2*k]=y;out[2*k+1]=0f
                    }
                    var offset=0;while(offset<n&&running){val written=track.write(out,offset,n-offset,AudioTrack.WRITE_BLOCKING);require(written>0){"Playback failed ($written)"};offset+=written}
                    require(track.underrunCount==0){"Playback underrun invalidated path timing"}
                }
                if(recordProbe){val f=File(context.getExternalFilesDir(null),"probe_response.csv");f.bufferedWriter().use{writer->writer.write("probe,response\n");for(k in 0 until count)writer.write("${probe[k]},${response[k]}\n")};status="Probe saved: ${f.absolutePath}\nRoute token: $token\nSamples: $count. Validate before use."}
                else status="Fixture stopped after $count samples. No acoustic attenuation claimed."
            }catch(e:Exception){status="Stopped: ${e.message}"}
            finally{running=false;recorder=null;try{record?.stop()}catch(_:Exception){};try{track?.stop()}catch(_:Exception){};record?.release();track?.release();manager.unregisterAudioDeviceCallback(routeCallback);onComplete(status)}
        }.also{it.name="ANC fixture";it.start()}
    }
    fun stop(){running=false;try{recorder?.stop()}catch(_:Exception){};thread?.join(2000);if(thread?.isAlive!=true)thread=null}
}
