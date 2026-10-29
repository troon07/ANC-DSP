$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/android_environment.ps1"
$cacheRoot = "$env:USERPROFILE/.gradle/caches/modules-2/files-2.1"
$compiler = Get-ChildItem "$cacheRoot/org.jetbrains.kotlin/kotlin-compiler-embeddable/1.9.22" -Filter '*.jar' -Recurse | Where-Object { $_.Name -notmatch 'sources|javadoc' } | Select-Object -First 1
$stdlib = Get-ChildItem "$cacheRoot/org.jetbrains.kotlin/kotlin-stdlib/1.9.22" -Filter '*.jar' -Recurse | Where-Object { $_.Name -notmatch 'sources|javadoc' } | Select-Object -First 1
$supportJars = @($compiler.FullName, $stdlib.FullName)
foreach ($artifactPath in @('org.jetbrains.kotlin/kotlin-script-runtime','org.jetbrains.kotlin/kotlin-reflect','org.jetbrains.intellij.deps/trove4j','org.jetbrains/annotations')) {
    $jar = Get-ChildItem "$cacheRoot/$artifactPath" -Filter '*.jar' -Recurse | Where-Object { $_.Name -notmatch 'sources|javadoc' } | Select-Object -First 1
    if ($jar) { $supportJars += $jar.FullName }
}
if (-not $compiler -or -not $stdlib) { throw 'Run the Android Gradle build first to populate Kotlin compiler dependencies' }
$outputDir = "$projectRoot/build_verified/kotlin_dsp_test"
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
& "$env:JAVA_HOME/bin/java.exe" -cp ($supportJars -join ';') org.jetbrains.kotlin.cli.jvm.K2JVMCompiler -no-stdlib -no-reflect -jvm-target 17 -classpath ($supportJars -join ';') -d $outputDir "$projectRoot/apps/android/app/src/main/java/com/universalanc/FxController.kt" "$projectRoot/apps/android/app/src/main/java/com/universalanc/TonalController.kt" "$projectRoot/apps/android/app/src/main/java/com/universalanc/AdaptiveDsp.kt" "$projectRoot/apps/android/dsp_test/Main.kt"
if ($LASTEXITCODE -ne 0) { throw 'Kotlin DSP test compilation failed' }
& "$env:JAVA_HOME/bin/java.exe" "-Danc.traceDirectory=$projectRoot/results/full_dsp/cpp" -cp "$outputDir;$($stdlib.FullName)" MainKt
if ($LASTEXITCODE -ne 0) { throw 'Kotlin DSP assertions failed' }
