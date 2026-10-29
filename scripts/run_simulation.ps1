$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
& "$projectRoot/build_verified/tools/simulator/anc_simulator.exe" all "$projectRoot/results/cpp"
if ($LASTEXITCODE -ne 0) { throw 'C++ simulations failed' }
