$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
cmake -S $projectRoot -B "$projectRoot/build_verified" -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
cmake --build "$projectRoot/build_verified" --parallel
if ($LASTEXITCODE -ne 0) { throw 'C++ build failed' }
