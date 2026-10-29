$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
ctest --test-dir "$projectRoot/build_verified" --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'DSP tests failed' }
