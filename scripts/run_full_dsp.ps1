param([switch]$SkipMatlab,[string]$MatlabExecutable='')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot/build_windows.ps1"
& "$PSScriptRoot/run_tests.ps1"
& "$projectRoot/build_verified/tools/simulator/anc_adaptive.exe" --benchmark "$projectRoot/results/full_dsp/cpp"
if($LASTEXITCODE -ne 0){throw 'Adaptive DSP benchmark failed'}
if(-not $SkipMatlab){
    if(-not $MatlabExecutable){$installed=Get-ChildItem 'C:/Program Files/MATLAB' -Directory | Sort-Object Name -Descending | Select-Object -First 1;$MatlabExecutable=Join-Path $installed.FullName 'bin/matlab.exe'}
    Push-Location $projectRoot
    try{$run=Start-Process -FilePath $MatlabExecutable -ArgumentList '-batch','"addpath(''matlab''); anc_full_dsp;"','-logfile','results_full_dsp.log' -WindowStyle Hidden -PassThru;$run.WaitForExit();if($run.ExitCode -ne 0){throw 'MATLAB full DSP checks failed; see results_full_dsp.log'}}finally{Pop-Location}
}
Write-Host "Full DSP results: $projectRoot/results/full_dsp/matlab/REPORT.md"
