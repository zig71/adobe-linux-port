# ls_webview.ps1 — inventory the runtime dir tree on the oracle.
$ErrorActionPreference = 'SilentlyContinue'
$d = 'C:\Program Files (x86)\Microsoft\EdgeWebView\Application'
Write-Output "=== top ==="
Get-ChildItem $d | Select-Object Name, @{n='Kind';e={if ($_.PSIsContainer) {'dir'} else {'file'}}} | Format-Table -AutoSize | Out-String
Write-Output "=== 153.0.4234.32 top ==="
Get-ChildItem (Join-Path $d '153.0.4234.32') | Select-Object Name | Format-Table -AutoSize -HideTableHeaders | Out-String
Write-Output "LS_DONE"
