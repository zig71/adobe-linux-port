# win_state.ps1 — installer window state on the oracle, no pixels needed.
$ErrorActionPreference = 'SilentlyContinue'
Get-Process | Where-Object { $_.ProcessName -match 'Creative|msedgewebview2' } |
    Select-Object ProcessName, Id, SessionId, MainWindowTitle,
        @{n='HasWindow';e={$_.MainWindowHandle -ne 0}} |
    Format-Table -AutoSize | Out-String
Write-Output "WINSTATE_DONE"
