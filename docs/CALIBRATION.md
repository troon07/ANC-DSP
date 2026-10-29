# Calibration and measurement

1. Build a bench fixture per `HARDWARE_SETUP.md`. Mute program audio and all OS audio effects. Fix gain, geometry, sample rate and stream configuration.
2. List Windows endpoints with `universal_anc_win --devices`. Record with:

   `universal_anc_win --record-probe recording.csv "CAPTURE_ID" "RENDER_ID" 48000 10`

   Probe amplitude is +/-0.02 FS. CSV uses output probe and the error microphone on input 1 at the software sample timeline. The full delay is represented by leading FIR zeros. The app prints the exact route token `CAPTURE_ID|RENDER_ID`.
3. Identify and validate:

   `anc_measurement recording.csv 48000 2048 "CAPTURE_ID|RENDER_ID" calibrated.profile calibration_report.csv`

   2048 taps span 42.67 ms at 48 kHz. Increase only when the captured impulse response needs it (max 4096). Training is first 75%; held-out validation last 25%. A threshold alone is not sufficient: inspect tap delay, residual and repeat recordings for clock drift. Repeatability is required because host IO delay may change between stream opens. Reject variable-delay routes; this prototype has no fractional delay tracking or automatic drift compensation.
4. Run control for a fixed interval:

   `universal_anc_win --fixture calibrated.profile "CAPTURE_ID" "RENDER_ID" 48000 10`

   Only output channel 0 is active. A discontinuity, underrun or invalid sample stops the loop. If startup immediately trips, inspect native format and stream timing; do not bypass trip checks.
5. For actual attenuation, record baseline with ANC off and then residual with ANC on under the SAME source, gain, position and fit. Compare matched error-sensor power, spectra and calibrated SPL. Reference-vs-error RMS is not attenuation. The current live app does not automatically collect a matched ANC-off/on report: acquire it independently on the bench.

The supplied `results/matlab/identified.profile` has `route=simulation` and is intentionally unusable for live output. The old recovered JSON profile was an example, not proof of a measured headset calibration; it is archived under `recovery/recovered_examples/`.

Profile text format:

```
ANC_PROFILE_V1 48000 ROUTE_TOKEN NUMBER_OF_TAPS
tap_0
tap_1
...
```

Route tokens contain no whitespace. The calibration application stores sample rate and endpoint identity; changing any gain or acoustic fit requires recalibration even if IDs remain the same.
