package com.universalanc
import android.Manifest
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Bundle
import android.widget.*
import android.media.AudioDeviceInfo

class MainActivity: Activity() {
    private lateinit var status: TextView;private lateinit var engine: AncAudioEngine
    private lateinit var inputSpinner: Spinner;private lateinit var outputSpinner: Spinner
    private var inputs=listOf<AudioDeviceInfo>();private var outputs=listOf<AudioDeviceInfo>();private var profile: AncProfile?=null
    private var alive=true
    private lateinit var tonal:TonalAudioEngine
    private lateinit var toneFrequency:EditText
    override fun onCreate(savedInstanceState: Bundle?){
        super.onCreate(savedInstanceState);window.addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);setContentView(com.universalanc.R.layout.activity_main);engine=AncAudioEngine(this);tonal=TonalAudioEngine(this)
        val layout=findViewById<LinearLayout>(R.id.content)
        val padding=(24*resources.displayMetrics.density).toInt()
        layout.setOnApplyWindowInsetsListener { view,insets ->
            if(android.os.Build.VERSION.SDK_INT>=30){val bars=insets.getInsets(android.view.WindowInsets.Type.systemBars());view.setPadding(padding+bars.left,padding+bars.top,padding+bars.right,padding+bars.bottom)}
            else {@Suppress("DEPRECATION") view.setPadding(padding+insets.systemWindowInsetLeft,padding+insets.systemWindowInsetTop,padding+insets.systemWindowInsetRight,padding+insets.systemWindowInsetBottom)}
            insets
        }
        fun text(s:String,size:Float=16f):TextView {val t=TextView(this);t.text=s;t.textSize=size;t.setPadding(0,12,0,12);layout.addView(t);return t}
        text("UNIVERSAL ANC LAB",28f);text("DSP experiments · Android",18f)
        text("Experimental single-tone acoustic cancellation with wired/Bluetooth headphones. Bench test OUTSIDE your ears: turn manufacturer ANC/Ambient OFF and fix the LEFT earbud beside the phone microphone. Use a separate steady speaker source. This measures the microphone point, not universal ambient ANC at your ear.")
        fun button(label:String,action:()->Unit){val b=Button(this);b.text=label;layout.addView(b);b.setOnClickListener{try{action()}catch(e:Exception){status.text=e.message}}}
        status=text("Ready. Simulation produces no sound.");status.setTextColor(Color.rgb(12,80,90))
        button("Run DSP simulation") {status.text="Computing three experiments…";Thread{try{val result=AncSimulation.report();runOnUiThread{if(alive)status.text=result}}catch(e:Exception){runOnUiThread{if(alive)status.text=e.toString()}}}.start()}
        text("Tone frequency (100�1000 Hz; use the SAME frequency on the source device)")
        toneFrequency=EditText(this);toneFrequency.setText("300");toneFrequency.inputType=android.text.InputType.TYPE_CLASS_NUMBER;layout.addView(toneFrequency)
        text("Capture: choose PHONE microphone for tonal test; USB reference/error pair for FxLMS")
        inputSpinner=Spinner(this);layout.addView(inputSpinner);text("Control output: choose Nirvana Crystl headphone media route");outputSpinner=Spinner(this);layout.addView(outputSpinner)
        button("Refresh routes"){refresh()}
        button("START 32 s tonal cancellation test") {
            engine.stop();tonal.stop()
            if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED){requestPermissions(arrayOf(Manifest.permission.RECORD_AUDIO),1);status.text="Grant microphone permission, then press START again."}
            else {require(inputs.isNotEmpty()&&outputs.isNotEmpty());tonal.start(inputs[inputSpinner.selectedItemPosition],outputs[outputSpinner.selectedItemPosition],toneFrequency.text.toString().toDouble()){message->runOnUiThread{if(alive)status.text=message}}}
        }
        button("PLAY SOURCE TONE on phone speaker") {engine.stop();tonal.stop();tonal.startSpeakerTone(toneFrequency.text.toString().toDouble()){message->runOnUiThread{if(alive)status.text=message}}}
        button("SAVE tonal session ZIP") {require(tonal.latestSession!=null){"Complete or stop a tonal test first"};startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply{type="application/zip";putExtra(Intent.EXTRA_TITLE,"ANC-tonal-session.zip");addCategory(Intent.CATEGORY_OPENABLE)},3)}
        button("STOP ALL AUDIO") {engine.stop();tonal.stop();status.text="Output stopped. If a session was active, wait for its export message."}
        text("Advanced: calibrated two-sensor USB FxLMS fixture")
        button("Record 10 s calibration probe"){start(true)}
        button("Import calibrated .profile"){startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT).apply{type="*/*";addCategory(Intent.CATEGORY_OPENABLE)},2)}
        button("Run calibrated fixture for 10 s"){start(false)}
        button("Stop fixture"){engine.stop();status.text="Output stopped"}
        button("Export diagnostics"){status.text="Saved: ${AncDiagnostics(this).export(status.text.toString()).absolutePath}"}
        refresh()
    }
    private fun refresh(){
        engine.stop();tonal.stop();profile=null;val helper=AudioDeviceHelper(this);inputs=helper.inputs();outputs=helper.outputs()
        inputSpinner.adapter=ArrayAdapter(this,android.R.layout.simple_spinner_dropdown_item,inputs.map{helper.label(it)})
        outputSpinner.adapter=ArrayAdapter(this,android.R.layout.simple_spinner_dropdown_item,outputs.map{helper.label(it)})
        val micIndex=inputs.indexOfFirst{it.type==AudioDeviceInfo.TYPE_BUILTIN_MIC};if(micIndex>=0)inputSpinner.setSelection(micIndex)
        val headphoneIndex=outputs.indexOfFirst{it.productName.toString().contains("Nirvana",true)};if(headphoneIndex>=0)outputSpinner.setSelection(headphoneIndex)
        status.text="${inputs.size} inputs / ${outputs.size} outputs. Select phone microphone and headphone media output. Keep devices fixed during the 32 s test."
    }
    private fun start(probe:Boolean){
        engine.stop();tonal.stop()
        if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED){requestPermissions(arrayOf(Manifest.permission.RECORD_AUDIO),1);status.text="Grant microphone permission, then press the fixture action again.";return}
        require(inputs.isNotEmpty()&&outputs.isNotEmpty()){"No audio route"}
        engine.start(inputs[inputSpinner.selectedItemPosition],outputs[outputSpinner.selectedItemPosition],profile,probe){message->runOnUiThread{if(alive)status.text=message}}
        status.text="Bench fixture active · stop before changing route or fit"
    }
    @Deprecated("Platform activity result API")
    override fun onActivityResult(requestCode:Int,resultCode:Int,data:Intent?){super.onActivityResult(requestCode,resultCode,data)
        if(requestCode==3&&resultCode==RESULT_OK){val uri=data?.data;if(uri!=null){Thread{try{contentResolver.openOutputStream(uri)?.let{tonal.exportZip(it)}?:error("Cannot open destination");runOnUiThread{if(alive)status.text="Session ZIP saved. Copy it to the laptop and run MATLAB anc_compare_tonal."}}catch(e:Exception){runOnUiThread{if(alive)status.text="Export failed: ${e.message}"}}}.start()}}
        if(requestCode==2&&resultCode==RESULT_OK){try{profile=data?.data?.let{uri->contentResolver.openInputStream(uri)?.bufferedReader()?.use{AncProfile.parse(it.readText())}};status.text="Profile loaded for ${profile?.route}"}catch(e:Exception){profile=null;status.text="Profile rejected: ${e.message}"}}
    }
    override fun onStop(){engine.stop();tonal.stop();super.onStop()}
    override fun onDestroy(){alive=false;engine.stop();tonal.stop();super.onDestroy()}
}
