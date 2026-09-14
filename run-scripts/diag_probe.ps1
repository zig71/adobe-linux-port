# diag_probe.ps1 — check what happened with the session-1 probe run.
$ErrorActionPreference = 'Continue'
$dir = "$env:USERPROFILE\adobe-wine-lab\tests\build\probes"
Write-Output "=== dir ==="
Get-ChildItem $dir -Filter 'probe_wv2*' | Select-Object Name, Length | Format-Table -AutoSize | Out-String
Write-Output "=== out file? ==="
$out = Join-Path $dir 'probe_wv2_controller.session1.out'
if (Test-Path $out) { Write-Output ("bytes={0}" -f (Get-Item $out).Length); Get-Content $out | Select-Object -First 25 }
else { Write-Output "NO_OUT_FILE" }
Write-Output "=== probe procs? ==="
Get-Process probe_wv2_controller -ErrorAction SilentlyContinue | Select-Object Id, SessionId | Format-Table -AutoSize | Out-String
Write-Output "DIAG_DONE"
