# oracle_cleanup.ps1 — stop procmon, look for installer traces.
$ErrorActionPreference = 'SilentlyContinue'
& "$env:USERPROFILE\adobe-wine-lab\tools\Sysinternals\Procmon.exe" /Terminate | Out-Null
Start-Sleep -Seconds 2
Get-Process procmon* -ErrorAction SilentlyContinue | Stop-Process -Force
Write-Output "procmon stopped"
Get-Process Creative*,msedgewebview2 -ErrorAction SilentlyContinue |
    Select-Object ProcessName, Id, SessionId | Format-Table -AutoSize | Out-String
Write-Output "=== newest Adobe temp dirs ==="
Get-ChildItem "$env:TEMP\CreativeCloud" -Recurse -ErrorAction SilentlyContinue |
    Sort-Object LastWriteTime -Descending | Select-Object -First 5 FullName, LastWriteTime |
    Format-Table -AutoSize | Out-String
Write-Output "CLEAN_DONE"
