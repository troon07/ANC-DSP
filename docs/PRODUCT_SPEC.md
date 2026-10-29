# Product scope: Universal ANC Lab 2.0

Deliver a reproducible DSP research prototype on Windows and Android, with MATLAB theory-to-results coverage. Simulation should work independently of audio hardware and report both successful and failed conditions. Live output is an explicit calibrated bench-fixture feature, not a universal consumer ANC switch. See `LIMITATIONS.md` for platform boundaries and `DSP_PROJECT_REPORT.md` for the actual implemented algorithms.

Acceptance: native desktop build/tests, eight reproducible simulation scenarios, executable MATLAB experiments with exported plots and numerical tables, a buildable Android app using independently checked DSP, transparent latency/capability reporting, calibrated profile gates and hardware test instructions. No arbitrary Bluetooth latency/attenuation target can be honestly guaranteed by host software alone.
