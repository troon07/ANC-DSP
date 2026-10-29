$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/android_environment.ps1"
& "$projectRoot/apps/android/gradlew.bat" -p "$projectRoot/apps/android" assembleDebug --console=plain
if ($LASTEXITCODE -ne 0) { throw 'Android build failed' }
New-Item -ItemType Directory -Path "$projectRoot/dist" -Force | Out-Null
Copy-Item -LiteralPath "$projectRoot/apps/android/app/build/outputs/apk/debug/app-debug.apk" -Destination "$projectRoot/dist/UniversalANC-Lab.apk" -Force
Write-Host "APK: $projectRoot/dist/UniversalANC-Lab.apk"
