package com.universalanc
import android.media.AudioDeviceInfo
import android.media.AudioManager
import android.content.Context
class AudioDeviceHelper(context: Context) {
    private val manager=context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
    fun inputs()=manager.getDevices(AudioManager.GET_DEVICES_INPUTS).toList()
    fun outputs()=manager.getDevices(AudioManager.GET_DEVICES_OUTPUTS).toList()
    fun label(d: AudioDeviceInfo)="${d.productName} (#${d.id}, type ${d.type}, ${d.channelCounts.joinToString("/")} ch)"
    fun usb(d: AudioDeviceInfo)=d.type==AudioDeviceInfo.TYPE_USB_DEVICE || d.type==AudioDeviceInfo.TYPE_USB_HEADSET
    fun token(input: AudioDeviceInfo,output: AudioDeviceInfo)="android:${input.id}|${output.id}"
    fun fixture(input: AudioDeviceInfo,output: AudioDeviceInfo)=usb(input)&&usb(output)&&input.channelCounts.any{it>=2}
}
