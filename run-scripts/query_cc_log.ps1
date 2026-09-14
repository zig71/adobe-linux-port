# query_cc_log.ps1 — read-only: report oracle installer state, OS identity, WebView2 key.
$ErrorActionPreference = 'SilentlyContinue'
$log = Join-Path $env:TEMP 'CreativeCloud\ACC\WAM.log'
Write-Output "=== WAM.log ==="
$f = Get-ChildItem $log
if ($f) {
    Write-Output ("exists len={0} mtime={1}" -f $f.Length, $f.LastWriteTime)
    $lines = Get-Content $log -ErrorAction SilentlyContinue
    Write-Output ("lines={0}" -f $lines.Count)
    Write-Output "--- states ---"
    $lines | Select-String 'state: ' | ForEach-Object { $_.Line } | Sort-Object -Unique | Select-Object -First 12
    Write-Output "--- onWindowResize count ---"
    @(Select-String -Path $log -Pattern 'onWindowResize').Count
    Write-Output "--- not-supported lines ---"
    Select-String -Path $log -Pattern 'not supported' | Select-Object -Last 3 | ForEach-Object { $_.Line.Substring(0, [Math]::Min(200, $_.Line.Length)) }
    Write-Output "--- last 4 INFO/WARN ---"
    $lines | Select-String '\[(INFO|WARN|ERROR)\]' | Select-Object -Last 4 | ForEach-Object { $_.Line.Substring(0, [Math]::Min(200, $_.Line.Length)) }
} else {
    Write-Output "NO_WAM_LOG"
}
Write-Output "=== OS identity (what installer checks) ==="
$cv = 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'
Get-ItemProperty $cv | Select-Object ProductName, DisplayVersion, CurrentBuild, CurrentBuildNumber, UBR, EditionID | Format-List | Out-String
Write-Output "=== WebView2 runtime key ==="
$wv = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}'
if (Test-Path $wv) { Get-ItemProperty $wv | Select-Object pv, name, location | Format-List | Out-String } else { Write-Output "MISSING_32" }
Write-Output "QUERY_DONE"
