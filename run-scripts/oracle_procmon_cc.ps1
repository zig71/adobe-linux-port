# oracle_procmon_cc.ps1 — capture the installer's reg/file activity on the
# oracle during CHECK_GENERAL_SYSTEM_REQUIREMENTS. Read-only vs host.
param(
    [string]$Exe = "$env:USERPROFILE\Downloads\Creative_Cloud_Set-Up.exe",
    [int]$Seconds = 75
)
$ErrorActionPreference = 'SilentlyContinue'
$lab = "$env:USERPROFILE\adobe-wine-lab"
$run = "$lab\runs\cc-oracle-procmon"
New-Item -ItemType Directory -Force -Path $run | Out-Null
$pm = "$lab\tools\Sysinternals\Procmon.exe"
$psexec = "$lab\tools\Sysinternals\PsExec64.exe"
$pml = "$run\cc.pml"
$csv = "$run\cc.csv"
Remove-Item $pml, $csv -ErrorAction SilentlyContinue

Write-Output "=== starting procmon ==="
& $pm /AcceptEula /Quiet /Minimized /BackingFile $pml | Out-Null
Start-Sleep -Seconds 3
Write-Output "=== launching installer in session 1 ==="
& $psexec -accepteula -nobanner -i 1 -d "$Exe" 2>&1 | Out-String | Select-Object -First 3
$wam = Join-Path $env:TEMP 'CreativeCloud\ACC\WAM.log'
for ($i = 0; $i -lt $Seconds; $i += 15) {
    Start-Sleep -Seconds 15
    $st = @(Select-String -Path $wam -Pattern 'state: ' -ErrorAction SilentlyContinue).Count
    Write-Output ("t+{0}s states={1}" -f ($i + 15), $st)
}
Write-Output "=== stopping procmon, exporting csv ==="
& $pm /Terminate | Out-Null
Start-Sleep -Seconds 3
& $pm /AcceptEula /Quiet /Minimized /OpenLog $pml /SaveAs $csv | Out-Null
Start-Sleep -Seconds 5
& $pm /Terminate | Out-Null
Get-ChildItem $run | Select-Object Name, @{n='MB';e={[math]::Round($_.Length/1MB,1)}} | Format-Table -AutoSize | Out-String
Write-Output "PROCMON_DONE"
