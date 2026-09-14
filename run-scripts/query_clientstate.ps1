# query_clientstate.ps1 — read-only: dump the EdgeUpdate ClientState key.
$ErrorActionPreference = 'SilentlyContinue'
foreach ($v in @('', '\WOW6432Node')) {
    $p = "HKLM:\SOFTWARE$($v)\Microsoft\EdgeUpdate\ClientState\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}"
    Write-Output "=== $p ==="
    if (Test-Path $p) { Get-ItemProperty $p | Format-List | Out-String }
    else { Write-Output "MISSING" }
}
Write-Output "CS_DONE"
