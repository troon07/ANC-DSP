# Architecture

`core/include/anc` holds fixed-buffer FIR, DC blocker, biquad, NLMS, normalized FxLMS, metrics, calibration, route-bound plant profiles and a guarded live engine. `simulation.h` defines eight deterministic acoustic models reused by simulator and desktop demo. Tests cover convergence, causality, model identification, filtering and trip behavior.

`apps/windows` uses native WASAPI shared-mode float formats with stereo capture. A worker captures two channels and queues one control channel to render. It rejects incompatible rates/formats and stops on discontinuity or render starvation. Capture/output should be one clock-synchronized USB interface. No resampling/drift compensation is implemented.

`apps/android` uses a Kotlin implementation of the same sample equations. Native JNI source is retained only as an optional C++ reference; the normal APK needs no NDK. A worker owns AudioRecord/AudioTrack, requires stereo USB input/output and a matching profile, requests unprocessed capture and low-latency output, and mutes on actual-route mismatch or underrun. UI and file export are outside the per-sample DSP path. It stops when the activity leaves the foreground.

`matlab` implements independent numerical experiments, identifies a synthetic plant, computes Welch PSD, writes all traces and exports 13 figures. Base MATLAB only. `scripts/generate_report.py` derives the report from actual result tables; the figures and workspace can be regenerated.

See `DSP_PROJECT_REPORT.md` for the signal-flow diagram and equations. This is single-channel ANC at one error sensor; stereo music mixing, system-wide audio capture, feedback-path estimation, adaptive sample-rate conversion and built-in headset ANC control are not implemented.

On Windows 10+ the backend requests the minimum supported shared engine period through IAudioClient3. Older interfaces use shared-mode initialization with a 20 ms buffer request. If the requested low period cannot initialize, opening fails rather than silently changing a calibrated path. This is a request, not proof of end-to-end acoustic latency.
