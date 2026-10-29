# Verification protocol

Run `scripts/run_project.ps1` from any directory. It fails on CMake/test/simulation/MATLAB errors. The MATLAB phase asserts nominal convergence, near-zero delayed broadband benefit, finite coefficients, no nominal output clipping, successful held-out identification and sample agreement with the C++ trace. `scripts/test_android_dsp.ps1` separately exercises the Kotlin DSP port on the JVM.

C++ deterministic suite: FIR impulse/circular-buffer order, DC rejection, IIR DC gain, NLMS plant convergence, FxNLMS >25 dB in a causal white-noise model, <3 dB with insufficient preview, route mismatch, simulation-profile rejection, nonfinite input muting, calibration held-out error/silent rejection and energy metrics. Safety tests exercise full-block mute after NaN, sustained clipping and bounded soft clipping.

MATLAB artifacts: summary.csv; eight 48,000-row traces; experiments.mat; bands.csv; calibration_recording.csv and validation summary; cpp_agreement.csv; fundamentals_summary.csv; 13 PNG/FIG plots; two equal-gain WAV files. Figure rendering must be visually inspected as well as numerical assertions.

Desktop app smoke tests: `--help`, `--demo`, `--devices`, invalid profile/mode fails nonzero. These do not send headphone output. Do not use default hardware for acoustic control without an independent two-sensor fixture. Bench acceptance requires repeatable actual round-trip latency, channel independence, plant validation on held-out recordings, matched ANC-off/on SPL spectra, no underruns/drift, stable route changes and gain fault tests.

Android acceptance: build debug APK, run Kotlin JVM causal/delay tests; then install on a physical phone, check layout/permission denial/background stop, USB preferred-vs-actual route validation, recording export, profile rejection and unplug mute. Device tests are still required; compiling does not prove mobile acoustic operation.
