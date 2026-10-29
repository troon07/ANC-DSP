# Capabilities and limits

| Target | Available | Validation |
|---|---|---|
| C++ DSP | FIR/IIR/DC, NLMS, normalized FxLMS, plant ID, gates | Compiled and deterministic tests run |
| MATLAB | Eight ANC scenarios, identification, 13 DSP plots, CSV/MAT/WAV | Executed in R2025b, base MATLAB |
| Windows | WASAPI discovery, silent demo, calibration probe, guarded fixture | Build, demo and actual device discovery checked; no acoustic fixture validated |
| Android | Kotlin simulations, route inspection, probe, profile import, timed USB fixture | See results report for build/JVM evidence; physical phone not tested |
| Wired headset | Simulation playback; live ANC only with added sensor fixture | No arbitrary-headphone attenuation claim |
| Wireless headset | New experimental single-tone phase-calibration mode at the laptop/phone microphone; simulation and diagnostics | Software tested; actual Crystl cancellation not yet measured; universal broadband ANC unavailable |
| iPhone/macOS/Linux host audio | Portable core source only | Host app/backend not implemented |

Model limits: one control channel, one error sensor, linear stationary FIR plant except one gain-change case, no acoustic feedback into reference, no clock drift/jitter, no music/double-talk protection, no real microphone/self-noise model, no spatial cancellation map. Controllers have no guaranteed global stability. Reported high reductions are idealized simulation results, not perceptual ratings, SPL guarantees or headphone specifications. Host audio is a research fixture path and does not establish universal low latency.

The recovered Git index and many objects are corrupt. The original archive and audit preserve evidence; no resets, history rewrite or index regeneration were performed. Source builds use a clean output directory independent of the corrupted old build.

The new live tonal mode uses one built-in error microphone and 32 seconds of OFF/probe/ON/OFF measurements. It is distinct from the two-sensor fixture and requires an independent steady source and fixed bench geometry. Read LIVE_TONAL_DEMO.md. Its addition does not establish ear-canal cancellation or universal compatibility.
