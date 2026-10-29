package com.universalanc
import android.annotation.SuppressLint
import android.content.Context
import android.media.*
import android.media.audiofx.*
import android.os.Process
import org.json.JSONObject
import java.io.File
import java.io.BufferedOutputStream
import java.io.FileOutputStream
import java.util.zip.ZipOutputStream
import java.util.zip.ZipEntry
import kotlin.math.*

class TonalAudioEngine(private val context:Context) {
    private val manager=context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
    @Volatile private var running=false
    @Volatile private var record:AudioRecord?=null
    @Volatile private var track:AudioTrack?=null
    private var worker:Thread?=null
    @Volatile var latestSession:File?=null;private set
    private var source=false
    private val routeCallback=object:AudioDeviceCallback(){override fun onAudioDevicesRemoved(devices:Array<out AudioDeviceInfo>){running=false;try{record?.stop()}catch(_:Exception){}}}
    @SuppressLint("MissingPermission")
    fun start(input:AudioDeviceInfo,output:AudioDeviceInfo,frequency:Double,update:(String)->Unit){
        require(worker?.isAlive!=true){"Stop the existing audio operation first"}
        require(input.type==AudioDeviceInfo.TYPE_BUILTIN_MIC){"Select the PHONE microphone, not the Bluetooth communications microphone"}
        require(output.type!=AudioDeviceInfo.TYPE_BUILTIN_SPEAKER&&output.type!=AudioDeviceInfo.TYPE_BUILTIN_EARPIECE){"Select headphone media output"}
        val controller=TonalController(frequency=frequency);val mic=FloatArray(controller.total.toInt());val control=FloatArray(mic.size)
        val directory=File(context.getExternalFilesDir(null),"tonal-${System.currentTimeMillis()}");require(directory.mkdirs()){"Cannot create recording folder"}
        latestSession=null;source=false;running=true;manager.registerAudioDeviceCallback(routeCallback,null)
        worker=Thread {
            Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO)
            val raw=manager.getProperty(AudioManager.PROPERTY_SUPPORT_AUDIO_SOURCE_UNPROCESSED)=="true"
            val effects=mutableListOf<AudioEffect>();var audioFault="none";var maxUs=0.0;var lastUi=0L;var underruns=0
            try {
                val rate=controller.rate
                val minIn=AudioRecord.getMinBufferSize(rate,AudioFormat.CHANNEL_IN_MONO,AudioFormat.ENCODING_PCM_FLOAT)
                val minOut=AudioTrack.getMinBufferSize(rate,AudioFormat.CHANNEL_OUT_STEREO,AudioFormat.ENCODING_PCM_FLOAT)
                require(minIn>0&&minOut>0){"48 kHz float recording/playback unsupported"}
                val recorder=AudioRecord.Builder().setAudioSource(if(raw)MediaRecorder.AudioSource.UNPROCESSED else MediaRecorder.AudioSource.VOICE_RECOGNITION).setAudioFormat(AudioFormat.Builder().setSampleRate(rate).setChannelMask(AudioFormat.CHANNEL_IN_MONO).setEncoding(AudioFormat.ENCODING_PCM_FLOAT).build()).setBufferSizeInBytes(maxOf(minIn*2,8192)).build()
                record=recorder
                require(recorder.state==AudioRecord.STATE_INITIALIZED&&recorder.setPreferredDevice(input)){"Phone microphone route unavailable"}
                if(AutomaticGainControl.isAvailable())AutomaticGainControl.create(recorder.audioSessionId)?.let{it.enabled=false;effects.add(it)}
                if(NoiseSuppressor.isAvailable())NoiseSuppressor.create(recorder.audioSessionId)?.let{it.enabled=false;effects.add(it)}
                if(AcousticEchoCanceler.isAvailable())AcousticEchoCanceler.create(recorder.audioSessionId)?.let{it.enabled=false;effects.add(it)}
                val player=AudioTrack.Builder().setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build()).setAudioFormat(AudioFormat.Builder().setSampleRate(rate).setChannelMask(AudioFormat.CHANNEL_OUT_STEREO).setEncoding(AudioFormat.ENCODING_PCM_FLOAT).build()).setBufferSizeInBytes(maxOf(minOut*2,16384)).setTransferMode(AudioTrack.MODE_STREAM).build()
                track=player;require(player.state==AudioTrack.STATE_INITIALIZED&&player.setPreferredDevice(output)){"Headphone route unavailable"}
                val buffer=FloatArray(480);val out=FloatArray(960)
                recorder.startRecording();player.play();val start=System.nanoTime()
                while(running&&!controller.done){
                    val n=recorder.read(buffer,0,buffer.size,AudioRecord.READ_BLOCKING);require(n>0){"Microphone read failed ($n)"}
                    require(recorder.routedDevice?.id==input.id){"Android changed the microphone route"}
                    if(System.nanoTime()-start>1_000_000_000L)require(player.routedDevice?.id==output.id){"Android did not honor the headphone media route"}
                    val begin=System.nanoTime()
                    for(k in 0 until n){var y=0f;if(!controller.done){val position=controller.count.toInt();y=controller.process(buffer[k]);mic[position]=if(buffer[k].isFinite())buffer[k] else 0f;control[position]=y};out[2*k]=y;out[2*k+1]=0f}
                    val us=(System.nanoTime()-begin)/1000.0;maxUs=max(maxUs,us)
                    var offset=0;while(offset<2*n&&running){val count=player.write(out,offset,2*n-offset,AudioTrack.WRITE_BLOCKING);require(count>0){"Headphone playback failed ($count)"};offset+=count}
                    underruns=player.underrunCount;require(underruns==0){"Playback underrun invalidated phase calibration"}
                    if(System.nanoTime()-lastUi>250_000_000L){lastUi=System.nanoTime();val live=controller.live
                        update("${TonalController.stageName(controller.stage)} · ${controller.count/rate} / 32 s\nMic RMS %.6f FS · tone %.7g FS²\nDSP %.1f µs/block (not round-trip latency)\n%s".format(sqrt(live.power),live.tone,us,if(raw)"Unprocessed capture requested" else "Capture processing is unverified; results cannot validate ANC"))}
                }
                if(!controller.done)controller.abort("user stopped before experiment completed")
            }catch(e:Exception){audioFault=e.message?:e.toString();controller.abort(audioFault)}
            finally {
                running=false;try{record?.stop()}catch(_:Exception){};try{track?.stop()}catch(_:Exception){};record?.release();track?.release();record=null;track=null;effects.forEach{try{it.release()}catch(_:Exception){}};manager.unregisterAudioDeviceCallback(routeCallback)
                try {
                    val n=controller.count.toInt();writeWave(File(directory,"microphone.wav"),mic,n,controller.rate);writeWave(File(directory,"control.wav"),control,n,controller.rate)
                    File(directory,"stages.csv").bufferedWriter().use{w->w.write("stage,tone_power,total_power,peak,sample_count\n");controller.points.forEachIndexed{i,p->w.write("$i,${p.tone},${p.power},${p.peak},${p.count}\n")}}
                    val before=(controller.points[5].tone+controller.points[7].tone)/2;val on=controller.points[6];val floor=max(1.0/(12*32767.0*32767.0),on.power-on.tone)*2/max(1L,on.count);val floorLimited=on.tone<floor;val reduction=TonalController.db(before,max(on.tone,floor))
                    val meta=JSONObject().put("schema_version",1).put("provenance","physical_microphone_recording").put("platform","android").put("sample_rate",controller.rate).put("frequency_hz",frequency).put("stage_seconds",4).put("settle_seconds",2).put("measurement_tail_seconds",0.1).put("sample_count",n).put("complete",controller.complete).put("reduction_observed",controller.complete&&reduction>=3&&raw).put("fault",controller.fault).put("input_name",input.productName.toString()).put("output_name",output.productName.toString()).put("input_id",input.id).put("output_id",output.id).put("probe_amplitude",0.02).put("candidate_real",controller.real).put("candidate_imag",controller.imag).put("model_valid",controller.modelValid).put("predicted_tone_power",controller.predictedTone).put("calibration_fit_error",controller.fitError).put("max_callback_us",maxUs).put("stream_faults",underruns).put("raw_capture_requested",raw).put("target_below_estimated_floor",floorLimited).put("audio_fault",audioFault).put("measured_round_trip_latency_ms",JSONObject.NULL).put("scope","single tone at microphone position; not verified at ear")
                    File(directory,"session.json").writeText(meta.toString(2));latestSession=directory
                    val result=if(controller.complete)"Measured target-tone level difference: %.2f dB at the phone microphone. %s %s".format(reduction,if(floorLimited)"Target below estimated floor." else "",if(raw)"" else "Processing unverified; ANC attribution is unvalidated.") else "No valid completed result: ${controller.fault}"
                    update("$result\nSession saved. Use SAVE SESSION ZIP for MATLAB comparison.")
                }catch(e:Exception){update("Export failed: ${e.message}")}
            }
        }.also{it.name="ANC tonal measurement";it.start()}
    }
    fun startSpeakerTone(frequency:Double,update:(String)->Unit){
        require(worker?.isAlive!=true){"Stop the existing audio operation first"};require(frequency.isFinite()&&frequency in 100.0..1000.0)
        val speaker=manager.getDevices(AudioManager.GET_DEVICES_OUTPUTS).firstOrNull{it.type==AudioDeviceInfo.TYPE_BUILTIN_SPEAKER}?:error("No built-in speaker")
        running=true;source=true
        worker=Thread {
            try {val rate=48000;val p=AudioTrack.Builder().setAudioAttributes(AudioAttributes.Builder().setUsage(AudioAttributes.USAGE_MEDIA).setContentType(AudioAttributes.CONTENT_TYPE_MUSIC).build()).setAudioFormat(AudioFormat.Builder().setSampleRate(rate).setChannelMask(AudioFormat.CHANNEL_OUT_MONO).setEncoding(AudioFormat.ENCODING_PCM_FLOAT).build()).setBufferSizeInBytes(16384).setTransferMode(AudioTrack.MODE_STREAM).build();track=p
                require(p.state==AudioTrack.STATE_INITIALIZED&&p.setPreferredDevice(speaker)){"Speaker routing request failed"};p.play();val buffer=FloatArray(480);var n=0L
                update("SOURCE TONE: $frequency Hz on phone speaker for up to 120 s. Run the LAPTOP test now. This mode performs no cancellation.")
                while(running&&n<rate*120L){for(k in buffer.indices){buffer[k]=(0.05*min(1.0,n/(rate*0.05))*cos(2*PI*frequency*n/rate)).toFloat();n++};var offset=0;while(offset<buffer.size&&running){val written=p.write(buffer,offset,buffer.size-offset,AudioTrack.WRITE_BLOCKING);require(written>0);offset+=written}
                    if(n>rate)require(p.routedDevice?.id==speaker.id){"Android routed the tone away from the phone speaker; disconnect earbuds from this source phone"}}
                update("Source tone stopped")
            }catch(e:Exception){update("Source stopped: ${e.message}")}
            finally{running=false;try{track?.stop()}catch(_:Exception){};track?.release();track=null;source=false}
        }.also{it.name="ANC reference source";it.start()}
    }
    fun stop(){running=false;try{record?.stop()}catch(_:Exception){};try{track?.pause();track?.flush()}catch(_:Exception){};worker?.join(2000);if(worker?.isAlive!=true)worker=null}
    fun exportZip(destination:java.io.OutputStream){val folder=latestSession?:error("Finish a measurement first");ZipOutputStream(BufferedOutputStream(destination)).use{zip->folder.listFiles()?.filter{it.isFile}?.forEach{f->zip.putNextEntry(ZipEntry(f.name));f.inputStream().use{it.copyTo(zip)};zip.closeEntry()}}}
    private fun writeWave(file:File,samples:FloatArray,n:Int,rate:Int){BufferedOutputStream(FileOutputStream(file)).use{o->
        fun u16(v:Int){o.write(v and 255);o.write(v ushr 8 and 255)};fun u32(v:Int){u16(v and 65535);u16(v ushr 16 and 65535)}
        o.write("RIFF".toByteArray());u32(36+n*2);o.write("WAVEfmt ".toByteArray());u32(16);u16(1);u16(1);u32(rate);u32(rate*2);u16(2);u16(16);o.write("data".toByteArray());u32(n*2)
        for(k in 0 until n)u16((samples[k].coerceIn(-1f,1f)*32767).roundToInt())}}
}
