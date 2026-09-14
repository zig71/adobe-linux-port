# find_cc.ps1 — is the installer alive on the oracle?
$ErrorActionPreference = 'SilentlyContinue'
Get-Process | Where-Object { $_.ProcessName -match 'Creative|Setup' } |
    Select-Object ProcessName, Id, SessionId, StartTime | Format-Table -AutoSize | Out-String
Write-Output "FIND_DONE"
