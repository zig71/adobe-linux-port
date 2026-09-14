# query Wow6432 view of CurrentVersion on the oracle.
$ErrorActionPreference = 'SilentlyContinue'
$p = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows NT\CurrentVersion'
if (Test-Path $p) {
    Get-ItemProperty $p | Select-Object ProductName, DisplayVersion, CurrentBuild, CurrentBuildNumber, UBR, EditionID, BuildLabEx | Format-List | Out-String
} else { Write-Output "NO_32BIT_VIEW" }
Write-Output "WOW_DONE"
