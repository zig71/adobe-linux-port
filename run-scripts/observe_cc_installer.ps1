# observe_cc_installer.ps1 — launch the Creative Cloud bootstrapper on the
# Windows oracle in the interactive session and record its observable effects.
#
# Read-only with respect to the host; the only mutation is whatever the
# installer itself performs inside the guest.
param(
    [string]$Exe = "$env:USERPROFILE\Downloads\Creative_Cloud_Set-Up.exe",
    [int]$Seconds = 90,
    [int]$SessionId = 1
)

$ErrorActionPreference = 'SilentlyContinue'
$ProgressPreference = 'SilentlyContinue'
$log = "$env:USERPROFILE\adobe-wine-lab\runs\cc-install-oracle"
New-Item -ItemType Directory -Force -Path $log | Out-Null

function Snapshot($tag) {
    $procs = Get-Process | Select-Object Id, ProcessName, SessionId, @{n='Path';e={$_.Path}},
        @{n='WS_MB';e={[math]::Round($_.WorkingSet64/1MB,1)}}
    $procs | Sort-Object ProcessName | Export-Csv -NoTypeInformation "$log\procs-$tag.csv"

    $paths = @(
        "$env:ProgramFiles\Adobe", "${env:ProgramFiles(x86)}\Adobe",
        "$env:ProgramData\Adobe", "$env:LOCALAPPDATA\Adobe",
        "$env:LOCALAPPDATA\Temp"
    )
    $files = foreach ($p in $paths) {
        if (Test-Path $p) {
            Get-ChildItem -Recurse -Force $p -ErrorAction SilentlyContinue |
                Select-Object @{n='Root';e={$p}}, FullName, Length, LastWriteTime
        }
    }
    $files | Export-Csv -NoTypeInformation "$log\files-$tag.csv"
    Write-Output "$tag processes=$($procs.Count) files=$($files.Count)"
}

Write-Output "=== session inventory ==="
Get-Process | Group-Object SessionId | ForEach-Object { "session $($_.Name): $($_.Count) processes" }

Write-Output "=== BEFORE snapshot ==="
Snapshot 'before'

Write-Output "=== launching in session $SessionId ==="
$psexec = "$env:USERPROFILE\adobe-wine-lab\tools\Sysinternals\PsExec64.exe"
$args = @('-accepteula', '-nobanner', '-i', "$SessionId", '-d', "`"$Exe`"")
Write-Output "cmd: $psexec $($args -join ' ')"
$out = & $psexec @args 2>&1 | Out-String
Write-Output $out
Write-Output "psexec_exit=$LASTEXITCODE"

for ($i = 0; $i -lt $Seconds; $i += 10) {
    Start-Sleep -Seconds 10
    $new = Get-Process -ErrorAction SilentlyContinue |
        Where-Object { $_.ProcessName -match 'Creative|CCX|Adobe|Acrobat|node|Setup|ACC|CoreSync|adobe' }
    Write-Output ("t+{0,3}s adobe-ish processes: {1}" -f ($i + 10),
        (($new | Select-Object -ExpandProperty ProcessName -Unique) -join ', '))
}

Write-Output "=== AFTER snapshot ==="
Snapshot 'after'

Write-Output "=== new processes vs before ==="
$before = Import-Csv "$log\procs-before.csv" | Select-Object -ExpandProperty Id
Import-Csv "$log\procs-after.csv" | Where-Object { $before -notcontains $_.Id } |
    Select-Object ProcessName, Id, SessionId, Path | Format-Table -AutoSize | Out-String

Write-Output "=== adobe-related processes now ==="
Get-Process | Where-Object { $_.ProcessName -match 'Creative|CCX|Adobe|Setup|ACC|CoreSync|node' } |
    Select-Object ProcessName, Id, SessionId, Path | Format-Table -AutoSize | Out-String

Write-Output "=== TCP connections ==="
Get-NetTCPConnection -State Established -ErrorAction SilentlyContinue |
    Select-Object -First 20 RemoteAddress, RemotePort, @{n='Owner';e={(Get-Process -Id $_.OwningProcess).ProcessName}} |
    Format-Table -AutoSize | Out-String

Write-Output "=== new files created ==="
$a = Import-Csv "$log\files-before.csv" | Select-Object -ExpandProperty FullName
Import-Csv "$log\files-after.csv" | Where-Object { $a -notcontains $_.FullName } |
    Select-Object -First 40 FullName, Length | Format-Table -AutoSize | Out-String

Write-Output "OBSERVE_DONE"
