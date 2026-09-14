# kill_cc_oracle.ps1 — clean up my wedged oracle runs.
$ErrorActionPreference = 'SilentlyContinue'
Get-Process Creative_Cloud_Set-Up -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 3
$left = @(Get-Process msedgewebview2 -ErrorAction SilentlyContinue).Count
Write-Output ("browser_children_left={0}" -f $left)
Write-Output "KILL_DONE"
