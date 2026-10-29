param([Parameter(Mandatory=$true)][string]$SessionPath)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot -Parent
$sessionFull=(Resolve-Path -LiteralPath $SessionPath).Path.Replace("'","''")
$matlabInstall=Get-ChildItem 'C:/Program Files/MATLAB' -Directory | Sort-Object Name -Descending | Select-Object -First 1
$matlabExe=Join-Path $matlabInstall.FullName 'bin/matlab.exe'
$sourcePath="$projectRoot/matlab".Replace("'","''")
$batchCommand="addpath('$sourcePath'); anc_compare_tonal('$sessionFull');"
$process=Start-Process -FilePath $matlabExe -ArgumentList '-batch',('"'+$batchCommand+'"') -WindowStyle Hidden -PassThru
$process.WaitForExit()
if($process.ExitCode -ne 0){throw 'MATLAB comparison failed or rejected the session'}
