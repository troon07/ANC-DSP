# Windows

Requirements: CMake 3.20+, Ninja and C++17 compiler (MSVC or MinGW). Run from any directory:

```powershell
D:\DSD\scripts\build_windows.ps1
D:\DSD\scripts\run_tests.ps1
D:\DSD\scripts\run_windows_app.ps1 --devices
D:\DSD\scripts\run_windows_app.ps1 --demo
```

Outputs: `build_verified/core/anc_tests.exe`, `build_verified/tools/simulator/anc_simulator.exe`, `build_verified/tools/measurement/anc_measurement.exe`, `build_verified/apps/windows/universal_anc_win.exe`. The old `build/` directory is preserved as original evidence and is not used. Fresh MinGW builds link runtimes statically for portable executable launch.

Live modes are explicit bench operations. First use `--devices` and copy the full endpoint IDs. Native stereo float formats and the requested sample rate must be supported; no implicit mono duplication is accepted. Follow `CALIBRATION.md` before running a fixture. Shared-mode buffer size is not a measured round-trip latency guarantee. Generic laptop microphones do not constitute a calibrated reference/error pair.

On Windows 10+ the backend requests the minimum supported shared engine period through IAudioClient3. Older interfaces use shared-mode initialization with a 20 ms buffer request. If the requested low period cannot initialize, opening fails rather than silently changing a calibrated path. This is a request, not proof of end-to-end acoustic latency.

## Version 2.1: desktop interface

Open `universal_anc_desktop.exe` from the new Windows package. It preselects detected Realtek/Nirvana endpoints and runs the single-error-microphone phase-calibration experiment. No Python runtime is needed for this GUI. MATLAB comparison is separate. Read LIVE_TONAL_DEMO.md before physical testing.
