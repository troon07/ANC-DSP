# Android

Run `scripts/build_android.ps1` or open `apps/android` in Android Studio with JDK 17, Android SDK 35 and Gradle 8.7. AGP 8.5.0 and Kotlin 1.9.22 are pinned; SDK 35 is accepted with its compatibility warning suppressed in `gradle.properties`. The standard build uses Kotlin DSP, so the NDK is not required. Debug APK: `apps/android/app/build/outputs/apk/debug/app-debug.apk`, copied to `dist/UniversalANC-Lab.apk`.

Install the debug APK manually. Run the simulation button: it should report low-delay reduction over 20 dB, and delayed broadband cases below 3 dB. No microphone permission or playback is needed for simulation. Route refresh lists available input/output IDs. Any wired/wireless route can be inspected; only stereo USB bench fixtures can arm control. Bluetooth stereo playback plus a mono headset microphone cannot supply a synchronized reference/error pair.

For calibration, select USB input/output, grant microphone permission and record the 10 s probe. Retrieve `probe_response.csv` from the app's external-files directory using Android Studio Device Explorer or `adb pull /sdcard/Android/data/com.universalanc/files/probe_response.csv`. Process it on the laptop at 48 kHz using `anc_measurement`, with the Android route token printed by the app. Choose enough FIR taps for the complete transport delay, up to 4096. Copy the resulting `.profile` back, import it through the system file picker, and run the timed fixture. Refresh, unplug or changing Android device IDs requires recalibration.

The UI stops the stream on backgrounding. No background recording, network permission or automatic external upload exists. Diagnostic text and probe recordings stay in the app's external-files directory. A debug build/JVM simulation check is not a physical phone/audio test; hardware timing and acoustic stability remain to be measured.

## Version 2.1: existing-device tonal demo

The current APK adds live single-tone calibration using the phone microphone and wired/Bluetooth media output, a separate built-in-speaker tone-source mode, and ZIP export for `anc_compare_tonal`. It is intended to run on Android 16, but no physical Android 16 device was connected during implementation. See [LIVE_TONAL_DEMO.md](LIVE_TONAL_DEMO.md).
