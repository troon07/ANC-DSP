from pathlib import Path
import zipfile
import hashlib
import json
ROOT=Path(__file__).resolve().parents[1]
dist=ROOT/'dist';dist.mkdir(exist_ok=True)
files={
    'universal_anc_desktop.exe':ROOT/'build_verified/apps/windows/universal_anc_desktop.exe',
    'reference_300Hz.wav':ROOT/'dist/reference_300Hz.wav',
    'matlab/anc_compare_tonal.m':ROOT/'matlab/anc_compare_tonal.m',
    'universal_anc_win.exe':ROOT/'build_verified/apps/windows/universal_anc_win.exe',
    'anc_simulator.exe':ROOT/'build_verified/tools/simulator/anc_simulator.exe',
    'anc_measurement.exe':ROOT/'build_verified/tools/measurement/anc_measurement.exe',
    'anc_tests.exe':ROOT/'build_verified/core/anc_tests.exe',
    'LICENSE':ROOT/'LICENSE',
}
for p in (ROOT/'docs').glob('*.md'): files['docs/'+p.name]=p
with zipfile.ZipFile(dist/'UniversalANC-Windows.zip','w',zipfile.ZIP_DEFLATED) as z:
    for name,p in files.items():z.write(p,name)
    z.writestr('README.md','# Universal ANC Lab for Windows\n\nExtract this ZIP and open `universal_anc_desktop.exe` for the live single-tone experiment. Read `docs/LIVE_TONAL_DEMO.md`. Use the Android app as the independent speaker source. `anc_tests.exe` verifies the DSP; `universal_anc_win.exe --demo` is a silent simulation. The demo produces no audio. Use `universal_anc_win.exe --devices` to inspect routes. Run `anc_simulator.exe all results` for eight CSV simulations. See START_HERE.txt and docs/CALIBRATION.md for bench-fixture instructions. Simulation results are not measured headphone attenuation. The experimental tonal mode attempts cancellation at one microphone point over wired/Bluetooth media routes; universal ambient ANC is not established. Full MATLAB source/results and the Android APK are provided separately in the project.\n')
    z.writestr('START_HERE.txt','Universal ANC Lab 2.1\n\nOpen universal_anc_desktop.exe for the LIVE SINGLE-TONE test.\nRead docs/LIVE_TONAL_DEMO.md and start the phone source first.\n\nRun anc_tests.exe to verify DSP.\nRun universal_anc_win.exe --demo for a silent simulation.\nRun universal_anc_win.exe --devices to inspect routes.\nRun anc_simulator.exe all results for CSV experiment traces.\n\nLive fixture modes need a calibrated two-microphone USB interface.\nRead docs/CALIBRATION.md before any acoustic experiment.\nThe tonal mode supports an experimental Bluetooth microphone-point test; universal headphone ANC is unverified. No universal headphone ANC claim.\nFull MATLAB results and source remain in the project workspace.\n')
artifacts=[]
for p in sorted(dist.glob('*')):
    if p.suffix in ['.apk','.zip']:artifacts.append({'name':p.name,'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()})
(dist/'checksums.json').write_text(json.dumps(artifacts,indent=2)+'\n')
print('\n'.join(str(dist/a['name']) for a in artifacts))
