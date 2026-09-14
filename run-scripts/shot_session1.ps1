# shot_session1.ps1 — run the screen grabber on the interactive desktop.
$ErrorActionPreference = 'Continue'
$psexec = "$env:USERPROFILE\adobe-wine-lab\tools\Sysinternals\PsExec64.exe"
& $psexec -accepteula -nobanner -i 1 powershell -NoProfile -ExecutionPolicy Bypass -File "$env:USERPROFILE\adobe-wine-lab\grab_screen.ps1" 2>&1 | Out-String
Write-Output "SHOT_DONE"
