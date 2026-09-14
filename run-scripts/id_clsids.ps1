# id_clsids.ps1 — read-only: name three CLSIDs from the oracle registry.
$ErrorActionPreference = 'SilentlyContinue'
foreach ($c in @('{daa52b27-8897-50af-ada5-c6c71bb64e17}',
                 '{33c53a50-f456-4884-b049-85fd643ecfed}',
                 '{54e211b6-3650-4f75-8334-fa359598e1c5}')) {
    Write-Output "=== $c ==="
    $p = "Registry::HKEY_CLASSES_ROOT\CLSID\$c"
    if (Test-Path $p) {
        Write-Output ("default={0}" -f (Get-ItemProperty $p -Name '(default)' -ErrorAction SilentlyContinue).'(default)')
        $s = Join-Path $p 'LocalServer32'
        if (Test-Path $s) { Write-Output ("localserver={0}" -f (Get-ItemProperty $s -Name '(default)').'(default)') }
        $s2 = Join-Path $p 'InProcServer32'
        if (Test-Path $s2) { Write-Output ("inproc={0}" -f (Get-ItemProperty $s2 -Name '(default)').'(default)') }
    } else { Write-Output "NOT_REGISTERED" }
}
Write-Output "ID_DONE"
