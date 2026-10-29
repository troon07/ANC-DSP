# Live demo with the laptop, Android 16 phone and Nirvana Crystl

This mode attempts **physical single-tone cancellation at the measuring microphone**. It is separate from the two-sensor FxLMS fixture. It does not establish ambient cancellation at a listener's ear. No physical cancellation result has been obtained on the user's equipment yet.

## First test: phone source, laptop controller

1. Extract `dist/UniversalANC-Windows.zip` to a writable folder and open `universal_anc_desktop.exe`. No Python or MATLAB is required to run this app.
2. Install `dist/UniversalANC-Lab.apk` on the Android phone. Android 16 is within the supported OS range; actual phone routing still needs to be checked. Installation is manual unless the phone is connected and authorized for development.
3. Connect both Crystl earbuds to the **laptop**, turn boAt ANC and Ambient modes OFF, and keep the earbuds active outside your ears. Disable wear-detection through the manufacturer's supported controls if needed. Do not wear the control earbud for this first bench test.
4. In the phone app, set **300 Hz** and tap **PLAY SOURCE TONE on phone speaker**. This emits a real, moderate-level tone for up to two minutes. Adjust physical source placement/volume so the laptop microphone can clearly hear it without clipping. The phone must remain in the foreground.
5. Position the **LEFT earbud** acoustic outlet next to the laptop microphone, without blocking either opening. Keep the phone and earbud fixed. A non-electronic support can help hold geometry steady. Cancellation will be measured only at this sensor point.
6. In the Windows app, select **Microphone (Realtek Audio), 48000 Hz** and **Headphones (Nirvana Crystl), 48000 Hz**. Do not select the 16 kHz Bluetooth headset microphone. The detected endpoints are preselected when their names match.
7. Press **Start 32-second test**. Do not move devices or change volume. The app shows live microphone RMS, target-tone power, phase and DSP callback time. **Stop and mute** ends output immediately through stream shutdown.
8. At the end, the app reports whether a completed test observed at least 3 dB of target-tone reduction or a rejection/failure. **Open session files** shows recordings and metadata under your Documents/UniversalANC/sessions folder. A failed calibration is not presented as successful ANC.

The Windows tonal mode requests RAW capture, matching native float sample rates, and an unchanged audio stream. If opening fails, verify OS microphone access yourself, select the laptop microphone and media headphone output, and check whether the driver supports RAW capture. The app does not bypass OS privacy permissions.

## Phone controller, laptop source

1. Connect the Crystl earbuds to the **phone**. Play the included `reference_300Hz.wav` through the **laptop's built-in speakers**, not through the earbuds. Select the speaker output in the OS/media player yourself. The WAV lasts two minutes and preserves a continuous tone.
2. In the phone app, refresh routes and select the **phone's built-in microphone** and **Nirvana Crystl media output**. Set 300 Hz. Place the LEFT earbud beside the phone's microphone and keep the source fixed. Keep the phone screen/app in the foreground.
3. Tap **START 32 s tonal cancellation test**, grant microphone permission when requested, and press START again. The app verifies actual input/output routes and stops if Android ignores or changes them.
4. A phone supporting unprocessed capture uses it. Otherwise voice-recognition capture is requested with available AGC/NS/AEC effects disabled. In that fallback the report explicitly marks microphone processing unverified; it cannot establish validated ANC attribution.
5. Use **SAVE tonal session ZIP** after the test. Copy the ZIP to the laptop and open it using the MATLAB comparison function below. Export only after recording has ended. The file picker/background transition stops any active audio operation.

Do not use simultaneous source and cancellation modes on the same phone. A device playing the independent source must not route that source into the control earbud. The source mode checks actual phone-speaker routing and rejects a mismatched route.

## What the automatic sequence does

| Time | Output state | Purpose |
|---|---|---|
| 0–4 s | OFF | Measure the target tone and baseline |
| 4–8 s | Small probe, 0 degrees | Identify interference response |
| 8–12 s | Small probe, 90 degrees | Identify interference response |
| 12–16 s | Small probe, 180 degrees | Identify interference response |
| 16–20 s | Small probe, 270 degrees | Fit and validate control amplitude/phase |
| 20–24 s | OFF | Verify source level before control |
| 24–28 s | Candidate ON | Measure the actual residual |
| 28–32 s | OFF | Check source drift after control |

Each stage excludes its first two seconds and last 0.1 s from the final measurement. Output transitions have 20 ms ramps. Probe amplitude is 0.02 FS. The candidate must remain below 0.08 FS. These are digital amplitude caps, not calibrated SPL limits.

The microphone must hear both the unwanted source and the control earbud. If the earbud is too far away, the necessary amplitude may exceed the cap; the test refuses rather than raising gain without bounds. Fixed Bluetooth delay is absorbed into a stationary phase calibration. Time-varying delay, frequency drift, movement, nonlinearities, wear-detection shutdown and capture processing can still prevent cancellation.

## Rejections and results

The controller rejects absent/weak tones, nonfinite/clipped microphone input, inconsistent phase observations, unidentifiable acoustic response, excessive required output, source level changes greater than 1 dB, and measured amplification during the candidate window. Stream errors, packet discontinuities and route changes stop output. These checks reduce confounding; they do not prove a general stable acoustic system or certify ear protection.

`session.json` records provenance, platform, selected devices, completion/fault state, candidate amplitude/phase, processing time and whether RAW capture was requested. `stages.csv` contains actual measured stage powers. `microphone.wav` and `control.wav` contain the recording and emitted digital waveform at a common software timeline. They are PCM16 at the native session rate, with unchanged relative gain. The driver/codec/acoustic latency is present in the physical recording and is not mislabeled as zero.

Target-tone values below an estimated measurement floor are reported as floor-limited, not as an exact arbitrarily deep null. Negative reduction means amplification. Callback processing time is not round-trip latency; actual round-trip latency remains null/unmeasured until independently acquired. Values are digital FS/dBFS or level differences, not calibrated dB SPL.

## MATLAB: actual recording versus identified simulation

```matlab
addpath('D:/DSD/matlab');
anc_compare_tonal('C:/path/to/tonal-session-folder');
% Or pass an exported ZIP; select its extracted folder on subsequent runs.
```

The function independently recomputes target-tone and total powers from the WAV, checks completion, gain/source drift and phase consistency, reconstructs a stationary phasor model from the OFF/probe observations, and predicts the candidate's residual. It writes `comparison.csv`, `COMPARISON.md`, `comparison.mat`, and editable/PNG figures. The model uses measured conditions; it is not the old idealized 8 kHz broadband simulation presented as a prediction for your headset.

If an experiment is incomplete or conditions fail validation, the report flags it and withholds an approved attenuation claim. A measured ON/OFF level difference can still be displayed as a diagnostic, without claiming it establishes ANC. Predictions can differ from measurements because the model assumes constant phase, level, geometry and linearity. The non-target noise floor is estimated from the recording.

## Software checks versus acoustic validation

C++ and Kotlin tests run the controller against a synthetic delayed acoustic loop and test silence/source-drift gates. The end-to-end C++/MATLAB session test is stored in `results/tonal_pipeline_test` with **synthetic** provenance and an explicit watermark. It cannot stand in for your actual headset recording. The MATLAB analyzer refuses synthetic input by default; its testing override is only for checking the export/analysis pipeline.

The actual product claim presently supported by the code is an **experimental single-tone controller and measurement tool**. Successful cancellation, if observed, applies to the microphone location and tested route. Universal headphone compatibility, moving listeners, arbitrary ambient noise and ear-canal attenuation remain unproven.
