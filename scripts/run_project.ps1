param([switch]$SkipMatlab, [string]$MatlabExecutable = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
& "$PSScriptRoot/build_windows.ps1"
& "$PSScriptRoot/run_tests.ps1"
& "$PSScriptRoot/run_simulation.ps1"
& "$projectRoot/build_verified/tools/simulator/anc_adaptive.exe" --benchmark "$projectRoot/results/full_dsp/cpp"
if ($LASTEXITCODE -ne 0) { throw 'Adaptive benchmark failed' }
if (-not $SkipMatlab) {
    if (-not $MatlabExecutable) {
        $matlabCommand = Get-Command matlab -ErrorAction SilentlyContinue
        if ($matlabCommand) { $MatlabExecutable = $matlabCommand.Source }
        else { $matlabInstall = Get-ChildItem 'C:/Program Files/MATLAB' -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1
            if ($matlabInstall) { $MatlabExecutable = Join-Path $matlabInstall.FullName 'bin/matlab.exe' }
        }
    }
    if (-not $MatlabExecutable -or -not (Test-Path -LiteralPath $MatlabExecutable)) { throw 'MATLAB not found: supply -MatlabExecutable or explicitly use -SkipMatlab' }
    Push-Location $projectRoot
    try {
        $matlabProcess = Start-Process -FilePath $MatlabExecutable -ArgumentList '-batch', '"addpath(''matlab''); anc_dsp_visualizer; anc_full_dsp;"', '-logfile', 'results_matlab_run.log' -WindowStyle Hidden -PassThru
        $matlabProcess.WaitForExit()
        if ($matlabProcess.ExitCode -ne 0) { throw 'MATLAB simulation/assertions failed; see results_matlab_run.log' }
    } finally { Pop-Location }
}
python "$PSScriptRoot/generate_report.py"
if ($LASTEXITCODE -ne 0) { throw 'Results report generation failed' }
Write-Host "Results: $projectRoot/results/REPORT.md"
