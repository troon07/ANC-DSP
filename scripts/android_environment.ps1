$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
if (-not $env:JAVA_HOME) {
    $localJava = Get-ChildItem "$projectRoot/tools/toolchains/jdk17" -Directory -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($localJava) { $env:JAVA_HOME = $localJava.FullName }
}
if (-not $env:JAVA_HOME -or -not (Test-Path "$env:JAVA_HOME/bin/java.exe")) { throw 'Set JAVA_HOME to a JDK 17 installation' }
$env:Path = "$env:JAVA_HOME/bin;" + $env:Path
if (-not $env:ANDROID_HOME) { $env:ANDROID_HOME = "$env:LOCALAPPDATA/Android/Sdk" }
if (-not (Test-Path "$env:ANDROID_HOME/platforms/android-35")) { throw 'Install Android SDK platform 35 and set ANDROID_HOME' }
