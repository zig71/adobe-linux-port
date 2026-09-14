# dump32view.ps1 — export the whole 32-bit CurrentVersion key.
$ErrorActionPreference = 'SilentlyContinue'
$p = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows NT\CurrentVersion'
Get-ItemProperty $p | Format-List | Out-String
Write-Output "DUMP_DONE"
