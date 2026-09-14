# oracle_check_seq.ps1 — read-only: what the oracle logs inside the check.
$ErrorActionPreference = 'SilentlyContinue'
$log = Join-Path $env:TEMP 'CreativeCloud\ACC\WAM.log'
$lines = Get-Content $log
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match 'CHECK_GENERAL_SYSTEM_REQUIREMENTS') {
        $a = [Math]::Max(0, $i - 2)
        $b = [Math]::Min($lines.Count - 1, $i + 14)
        Write-Output "=== context around line $i ==="
        $lines[$a..$b] | ForEach-Object { $_.Substring(0, [Math]::Min(190, $_.Length)) }
    }
}
Write-Output "SEQ_DONE"
