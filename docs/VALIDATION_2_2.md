# Version 2.2 validation

Executed on Windows, 29 October 2026. Results below describe software and synthetic signals. Physical phone execution and ear-level attenuation with boAt Nirvana Crystl remain unmeasured.

| Check | Result |
|---|---|
| Release C++17 Windows build | Passed: desktop GUI, CLI, simulator, adaptive processor and calibration tool |
| CTest algorithm and safety suite | Passed: filters, calibration, LMS/NLMS/RLS, FxNLMS, tonal model, reference/fault/watchdog guards |
| CTest processing allocation instrumentation | Passed: zero counted new/new[] allocations during tested DSP blocks |
| Independent MATLAB R2025b implementation | All 15 algorithm/scenario cases passed; largest residual difference from C++ 2.24e-5 FS |
| Kotlin DSP comparison | All 15 C++ traces matched within 2.24e-5 FS; convergence, delay, silence and nonfinite tests passed |
| Offline CSV processing | LMS/NLMS/RLS processed 8000 samples each; tail residual RMS below 0.001 FS for a correlated-noise fixture |
| Invalid offline inputs | Bad headers, nonfinite/empty data and input/output file identity rejected; original input preserved |
| Calibration CLI | Known delayed FIR identified at delay 3; held-out NMSE approximately -125 dB on the synthetic fixture |
| Calibration validation | Invalid rate/taps/route, clipped and nonfinite samples rejected |
| Existing eight acoustic simulations | Completed; low-delay model converged, modeled broadband transport delay did not provide cancellation |
| Windows CLI smoke test | Silent --demo completed; no physical output or attenuation claim |
| Android assembleDebug | Passed; installable debug APK generated, version 2.2, min API 26, target API 35 |
| Android lintDebug | Passed: 0 errors, 28 warnings, primarily localization and legacy backup configuration |

Reproduce with `scripts/run_full_dsp.ps1`, `scripts/build_android.ps1`, and `scripts/test_android_dsp.ps1`. Run Android lint with `apps/android/gradlew.bat -p apps/android lintDebug`. Build prerequisites are in the platform build documentation. APK/ZIP hashes are in `dist/checksums.json`.

The generic Android LMS/NLMS/RLS module is a reusable digital processing library. The app's existing live modes remain the calibrated FxNLMS fixture and experimental single-tone test. An arbitrary headset does not expose the independent synchronized reference/error signals needed to validate general acoustic ANC. Use `docs/LIVE_TONAL_DEMO.md` for the available physical experiment and `anc_compare_tonal` for exported recordings; those recordings are required before reporting measured improvement.
