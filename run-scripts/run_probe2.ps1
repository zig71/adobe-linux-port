# run_probe2.ps1 — session-1 probe with exit code capture.
$ErrorActionPreference = 'Continue'
$psexec = "$env:USERPROFILE\adobe-wine-lab\tools\Sysinternals\PsExec64.exe"
$dir = "$env:USERPROFILE\adobe-wine-lab\tests\build\probes"
$out = Join-Path $dir 'probe_wv2_controller.session1.out'
Remove-Item $out -ErrorAction SilentlyContinue
Write-Output "=== sessions ==="
query user 2>&1 | Out-String
Write-Output "=== launch ==="
$o = & $psexec -accepteula -nobanner -i 1 "$dir\probe_wv2_controller.exe" 2>&1 | Out-String
Write-Output $o
Write-Output ("psexec_exit={0}" -f $LASTEXITCODE)
Start-Sleep -Seconds 90
Write-Output "=== output bytes + content ==="
if (Test-Path $out) { Write-Output ("bytes={0}" -f (Get-Item $out).Length); Get-Content $out | Select-Object -First 25 }
else { Write-Output "NO_OUT_FILE" }
Write-Output "RUN2_DONE"
