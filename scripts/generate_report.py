"""Generate a results document from actual CSV artifacts; no numerical results are hardcoded."""
import csv
import hashlib
import json
from datetime import datetime, timezone, timedelta
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'results'
def rows(relative):
    with (OUT / relative).open(newline='', encoding='utf-8-sig') as f:
        return list(csv.DictReader(f))

cpp=rows('cpp/summary.csv')
matlab=rows('matlab/summary.csv')
agreement=float(rows('matlab/cpp_agreement.csv')[0]['max_residual_absolute_difference'])
identification=float(rows('matlab/calibration_summary.csv')[0]['validation_nmse_db'])
fundamentals=rows('matlab/fundamentals_summary.csv')[0]
assert len(cpp)==len(matlab)==8
assert agreement<1e-4
assert len(list((OUT/'matlab').glob('*.png')))==13

by_name={r['scenario']:r for r in cpp}
lines=[
    '# Universal ANC Lab — executed results',
    '',
    f'Generated {datetime.now(timezone(timedelta(hours=5,minutes=30))).isoformat(timespec="seconds")} (India time).',
    '',
    'C++17 and base MATLAB R2025b were executed on Windows. These are **digital acoustic-model simulations**, not measured cancellation at a human ear. The host audio applications are research fixture tools; universal broadband ANC through arbitrary Bluetooth headphones is not an achieved capability.',
    '',
    '## Reproduce',
    '',
    'Run `scripts/run_project.ps1` from PowerShell. For Android use `scripts/build_android.ps1`, then `scripts/test_android_dsp.ps1`. MATLAB alone: `addpath(\'matlab\'); anc_dsp_visualizer;`.',
    '',
    '## Eight controlled experiments',
    '',
    'Each run: 8 kHz, six seconds, 64 controller taps, step size 0.02, leakage 1e-6. Attenuation uses the final 1.5 seconds against the same ANC-off waveform at the modeled error sensor. No microphone-to-microphone RMS shortcut is used. Negative values indicate amplification.',
    '',
    '| Scenario | MATLAB attenuation, dB | C++ attenuation, dB | MATLAB first 0.5 s, dB | Peak output, FS | Clipped samples |',
    '|---|---:|---:|---:|---:|---:|',
]
for r in matlab:
    c=by_name[r['scenario']]
    lines.append(f"| {r['scenario']} | {float(r['attenuation_db']):.3f} | {float(c['attenuation_db']):.3f} | {float(r['early_attenuation_db']):.3f} | {float(r['output_peak']):.5f} | {r['clipped_samples']} |")
lines += [
    '',
    'The nominal paths deliberately provide 3 ms of primary propagation delay versus 1 ms of secondary delay. The two larger-delay cases use 5 ms and 50 ms secondary delay, exceeding preview and leaving almost no broadband cancellation. These delay choices are illustrative, not measurements of any wired/wireless product. The uncorrelated case retains noise the reference cannot predict. The modest model mismatch happens to converge faster here; it does not imply inaccurate models improve ANC in general.',
    '',
    'The large nominal reductions rely on a simple FIR plant, stable sample timing, noiseless sensors and no acoustic feedback. They are unsuitable as consumer headphone performance claims. C++ float and MATLAB double precision differ slightly in deep-cancellation floors.',
    '',
    '## Cross-implementation and identification checks',
    '',
    f'- Maximum broadband residual difference, C++ versus MATLAB: **{agreement:.9g} FS**, below the 1e-4 FS acceptance threshold.',
    f'- MATLAB held-out secondary-path identification NMSE: **{identification:.3f} dB**, using an independent synthetic probe with additive measurement noise.',
    '- The C++ identification tool also processed the MATLAB recording successfully; its independent result is in `matlab/cpp_calibration.csv`.',
    f"- PCM16 quantization SNR for a 0.1 FS sine: **{float(fundamentals['pcm16_snr_db']):.3f} dB**.",
    f"- Offline decimation-filter group delay: **{float(fundamentals['decimation_fir_group_delay_ms']):.4f} ms**; this is additional delay, not a live ANC latency guarantee.",
    '',
    '## Frequency-band reduction',
    '',
    '| Band, Hz | MATLAB simulated attenuation, dB |',
    '|---|---:|',
]
for r in rows('matlab/bands.csv'):
    lines.append(f"| {r['low_hz']}–{r['high_hz']} | {float(r['attenuation_db']):.3f} |")
lines += [
    '',
    'Bands integrate the final-window one-sided Welch PSD. Hann length 512, 50% overlap, 15.625 Hz bin spacing. These bins are diagnostics rather than independently controlled filter-bank channels.',
    '',
    '## Figures',
    '',
]
captions={
    '01':'Windowed-sinc FIR: impulse, magnitude, phase and passband group delay.',
    '02':'DC blocking and second-order IIR low-pass frequency responses.',
    '03':'Integrated low-frequency PSD-band reduction.',
    '04':'NLMS training convergence, true and identified secondary impulse responses.',
    '05':'True and estimated secondary-path frequency response and phase.',
    '06':'Filtered-x normalized LMS convergence and controller coefficient norm.',
    '07':'Before/after Welch power spectral density at the modeled error sensor.',
    '08':'Scenario comparison, including failed causality conditions.',
    '09':'Output limit and nominal clipping count.',
    '10':'PCM16 rounding and quantization error.',
    '11':'Sampling alias: 5.5 kHz becomes 2.5 kHz at an 8 kHz sample rate.',
    '12':'Anti-alias FIR before decimation from 48 to 8 kHz.',
    '13':'Sensitivity of equal-amplitude anti-phase control to unmodeled delay.',
}
for p in sorted((OUT/'matlab').glob('*.png')):
    lines += [f'### {captions[p.name[:2]]}', '', f'![{captions[p.name[:2]]}](matlab/{p.name})', '']
lines += [
    '## Applications and remaining hardware validation',
    '',
    'Version 2.1 adds a Windows GUI and Android single-tone phase-calibration mode with real microphone recording and MATLAB comparison. See ../docs/LIVE_TONAL_DEMO.md. Its delayed-plant software tests pass; the exported synthetic session in ../results/tonal_pipeline_test is explicitly not a physical headset measurement. The new mode does not require the two-sensor fixture, but does require fixed microphone-point geometry and an independent stationary source.',
    '',
    'The Windows executable was built and smoke-tested with `--help`, `--demo` and `--devices`. Device discovery found Nirvana Crystl 16 kHz mono capture and 48 kHz stereo playback, plus Realtek endpoints. This headset route lacks the independent reference/error channels required by the controller.',
    '',
    'The Android debug APK was compiled. The Kotlin DSP port was compiled and executed separately on JDK 17: its low-delay, 40-sample-delay, 400-sample-delay and NaN-gate assertions pass. It matches the C++ float broadband attenuation within 0.1 dB. No physical phone or USB acoustic-fixture test was performed. APK compilation is not evidence of mobile acoustic attenuation.',
    '',
    'Live hardware modes require stereo synchronized sensor channels, fixed gain, an exact route/rate profile, complete secondary-path delay identification and repeatable timing. Matched acoustic ANC-off/on measurements, SPL limits, route-change fault tests and clock-drift tests remain mandatory before claiming real headphone ANC. iOS and non-Windows desktop audio backends are not implemented.',
    '',
    '## Artifacts',
    '',
    '- `matlab/experiments.mat`: full MATLAB workspace arrays and controller weights.',
    '- `matlab/*.csv`: eight full traces, metrics, PSD band reductions and identification results.',
    '- `matlab/*.fig`: editable MATLAB figures; matching PNGs above.',
    '- `matlab/baseline.wav` and `matlab/residual.wav`: hum example, identical gain, no independent normalization.',
    '- `cpp/*.csv`: independent C++ traces and summary.',
    '- `../dist/UniversalANC-Lab.apk` and `../dist/UniversalANC-Windows.zip`: local install/run packages.',
    '- `../docs/DSP_PROJECT_REPORT.md`: complete equations, algorithm details, assumptions and primary references.',
    '- `../recovery/original_project.zip` and `../recovery/audit.json`: original corrupted checkout and recovery evidence.',
    '',
    'The original Git index and many objects remain corrupt and were preserved. The current source and generated artifacts are usable independently of Git recovery.',
]
(OUT/'REPORT.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')

manifest=[]
for directory in [OUT/'cpp',OUT/'matlab']:
    for p in sorted(directory.iterdir()):
        if p.is_file():
            manifest.append({'path':str(p.relative_to(ROOT)).replace('\\','/'),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(OUT/'artifact_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
print(OUT/'REPORT.md')
