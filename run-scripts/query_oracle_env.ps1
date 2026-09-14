# oracle_env.ps1 — read-only: machine profile the requirements gate may use.
$ErrorActionPreference = 'SilentlyContinue'
Write-Output "=== clock ==="
Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
Write-Output "=== WAM.log still the old run? ==="
$log = Join-Path $env:TEMP 'CreativeCloud\ACC\WAM.log'
(Get-ChildItem $log).LastWriteTime
Write-Output "=== locale ==="
Get-WinSystemLocale | Select-Object -ExpandProperty Name
Get-Culture | Select-Object -ExpandProperty Name
Write-Output "=== memory ==="
[math]::Round((Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory/1GB,1)
Write-Output "=== cpu ==="
Get-CimInstance Win32_Processor | Select-Object Name, NumberOfCores, NumberOfLogicalProcessors | Format-List | Out-String
Write-Output "=== disk free C: ==="
[math]::Round((Get-PSDrive C).Free/1GB,1)
Write-Output "=== os ==="
Get-CimInstance Win32_OperatingSystem | Select-Object Caption, Version, BuildNumber, OSArchitecture, SuiteMask, ProductType | Format-List | Out-String
Write-Output "ENV_DONE"
