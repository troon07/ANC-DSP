# Safety and data

Calibration and live control are bench-fixture operations. Keep listeners out of the fixture until calibrated SPL, stability and fault handling are independently established. A 0.2 FS digital cap can still produce high SPL depending on amplifier sensitivity. Use physical gain limits and an accessible disconnect.

The controller mutes on nonfinite inputs/coefficients, sustained raw clipping or stream/route failure. Calibration profiles are explicit and route/rate bound; simulation profiles cannot arm the live loop. No error-energy-growth watchdog, SPL meter or medical/hearing-protection claim is implemented. Bluetooth/mono routes do not supply the required synchronized sensor channels.

Android requests microphone access only for fixture operations. It records a calibration CSV locally; app external-files storage and diagnostics can include endpoint identity and microphone data. No network permission or upload is included. Delete recordings through the OS when no longer needed. Windows reads/writes only explicit local files. Debug APK is for local testing, not store distribution.
