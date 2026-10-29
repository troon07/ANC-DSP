$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
& "$projectRoot/build_verified/apps/windows/universal_anc_win.exe" @args
if ($LASTEXITCODE -ne 0) { throw 'ANC application failed' }
