# ANC DSP

Universal ANC Lab: a DSP research project: C++17 adaptive controller, native Windows bench-fixture app, Android app, and reproducible base-MATLAB simulations. **Simulated attenuation is not measured headphone attenuation.** Universal acoustic cancellation through arbitrary wired/Bluetooth headphones is not physically achievable by a host app alone.

## Full adaptive DSP implementation

Run `scripts/run_full_dsp.ps1` for LMS, NLMS and RLS comparisons, conditioning, block APIs, acoustic FxNLMS guards and independent MATLAB verification. See [implementation and equations](docs/FULL_ADAPTIVE_DSP.md) and [executed full-DSP results](results/full_dsp/matlab/REPORT.md). Digital signal cleanup and physical acoustic anti-noise are explicitly separate.

## Live single-tone demo with your Crystl earbuds

The new Windows desktop app and Android app attempt a physical single-tone cancellation test using the existing laptop/phone microphone. Turn built-in headset ANC OFF, use one device as a steady speaker source, and fix the LEFT earbud beside the other device's microphone. Read [the exact setup and measurement procedure](docs/LIVE_TONAL_DEMO.md). This is cancellation at a sensor point, not proven ambient/ear-canal ANC.

Open `dist/UniversalANC-Windows.zip`, extract it, and launch `universal_anc_desktop.exe`. Install `dist/UniversalANC-Lab.apk` on the Android phone. MATLAB can analyze actual exported sessions with `anc_compare_tonal(sessionPath)`. A physical test requires your positioning and starting the source; no measured attenuation has been invented.

## Start here

```powershell
.\scripts\run_project.ps1
```

Builds the desktop project, runs deterministic tests and eight C++ simulations, runs MATLAB if installed, then writes `results/REPORT.md`. Requires CMake, a C++17 compiler and Ninja; MATLAB R2025b was used for the included results. No MATLAB toolboxes are required.

```powershell
.\scripts\run_windows_app.ps1 --devices
.\scripts\run_windows_app.ps1 --demo
.\scripts\build_android.ps1
```

The Android APK is copied to `dist/UniversalANC-Lab.apk`; install manually on an Android 8+ phone. The app runs silent DSP simulations, inspects wired/wireless audio routes, exports diagnostics, and provides gated USB bench calibration/control. Phone hardware execution has not been validated here. iOS, macOS and Linux host audio backends are not implemented; the C++ core and simulations are portable.

## Results and project material

- [Actual simulation results](results/REPORT.md): experiment definitions, measured numerical results, graphs, limitations and validation.
- [DSP project report](docs/DSP_PROJECT_REPORT.md): equations, block diagram, algorithm decisions, sampling, filtering, stability, latency and complexity.
- [Windows build and usage](docs/BUILD_WINDOWS.md), [Android build and usage](docs/BUILD_ANDROID.md).
- [Hardware setup](docs/HARDWARE_SETUP.md), [calibration procedure](docs/CALIBRATION.md), [test protocol](docs/TEST_PROTOCOL.md).
- [Capability limits](docs/LIMITATIONS.md), [recovery audit](docs/RECOVERY.md).

MATLAB entry: `addpath('matlab'); anc_dsp_visualizer;`. Outputs are in `results/matlab/`: 13 PNG and editable FIG plots, eight sample-level CSV traces, summary/band/calibration tables, a MAT workspace and matched-gain baseline/residual WAV files.

The live fixture requires a synchronized two-input interface: reference microphone on input 0, error microphone on input 1, control driver on output 0. A route-bound secondary-path profile must include the full capture/processing/playback delay. Ordinary headset microphones do not provide these two measurements. The new tonal mode performs a separate single-sensor phase-calibration experiment; Bluetooth is not supported by the two-sensor fixture mode. Bluetooth playback is also suitable for demonstrations; headphone-internal ANC must run inside the headphones.

The original checkout contained corrupted sources, images and Git objects. Original bytes were backed up before reconstruction to `recovery/original_project.zip`; `.git` was preserved and remains corrupt. Use the files directly; do not assume Git can restore them. See the recovery audit before attempting repository repair.

## GitHub snapshot

This repository was published as a clean initial commit. Legacy corrupt Git metadata, recovery archives, local toolchains, device diagnostics and physical microphone recordings were excluded. The original project and recovery archive remain on the local machine. The session files under `results/tonal_pipeline_test/` are explicitly synthetic test fixtures.
