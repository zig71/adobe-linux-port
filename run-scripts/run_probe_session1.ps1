# run_probe_session1.ps1 — run a probe on the interactive desktop, save output.
param([string]$Exe = "probe_wv2_controller.exe", [int]$WaitSecs = 100)
$ErrorActionPreference = 'SilentlyContinue'
$psexec = "$env:USERPROFILE\adobe-wine-lab\tools\Sysinternals\PsExec64.exe"
$dir = "$env:USERPROFILE\adobe-wine-lab\tests\build\probes"
$out = Join-Path $dir ($Exe -replace '\.exe$', '.session1.out')
Remove-Item $out -ErrorAction SilentlyContinue
& $psexec -accepteula -nobanner -i 1 "$dir\$Exe" > $out 2>&1
Start-Sleep -Seconds $WaitSecs
Write-Output "=== output ==="
Get-Content $out | Select-Object -First 25
Write-Output "SESSION1_DONE"
