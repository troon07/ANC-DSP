# Universal ANC build specification (reconstructed)

The original 18,956-byte specification was corrupted and is preserved byte-for-byte in `recovery/original_project.zip`. Its contents could not be read. This replacement records the scope implemented from the user's request and the surviving repository architecture.

Implement a portable C++17 DSP library with FIR/IIR/DC filters, normalized LMS plant identification, normalized filtered-x LMS ANC, metrics, stability gates and calibration profiles. Build Windows discovery/demo/probe/fixture tools and an Android simulation/USB-fixture app. Supply base-MATLAB simulations with actual exported results covering convergence, frequency response, PSD, sub-band power, identification, model mismatch, changing plant, uncorrelated noise, latency, sampling/aliasing, quantization and multirate filtering. Provide deterministic tests and reproducible build/run scripts.

The term universal describes reusable DSP and route diagnostics, not a guarantee of acoustic cancellation across arbitrary hardware. Broadband ANC requires adequate reference preview, a measured error signal and a sufficiently stable low-delay secondary path. Arbitrary wired/Bluetooth headphones generally do not expose the necessary microphones. Wireless native ANC belongs inside headphone firmware. iOS host apps and universal device-driver integration are not part of this implementation.

See `docs/PRODUCT_SPEC.md`, `docs/DSP_PROJECT_REPORT.md`, `docs/TEST_PROTOCOL.md` and `results/REPORT.md` for the concrete implementation and evidence.
