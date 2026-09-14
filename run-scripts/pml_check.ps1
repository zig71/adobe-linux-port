# pml_check.ps1 — did the installer run inside the capture?
$ErrorActionPreference = 'SilentlyContinue'
$pml = "$env:USERPROFILE\adobe-wine-lab\runs\cc-oracle-procmon\cc.pml"
Write-Output ("pml bytes={0}" -f (Get-Item $pml).Length)
Write-Output "=== session-1 browser cmdlines ==="
Get-CimInstance Win32_Process -Filter "Name='msedgewebview2.exe'" -ErrorAction SilentlyContinue |
    Select-Object ProcessId, @{n='Cmd';e={$_.CommandLine.Substring(0, [Math]::Min(150, $_.CommandLine.Length))}} |
    Format-Table -AutoSize | Out-String
Write-Output "CHECK_DONE"
