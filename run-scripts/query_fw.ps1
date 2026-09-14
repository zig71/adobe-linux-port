# query_fw.ps1 — SecureBoot / TPM / VBS state on the oracle.
$ErrorActionPreference = 'SilentlyContinue'
Write-Output "=== secureboot ==="
try { Confirm-SecureBootUEFI } catch { Write-Output ("err={0}" -f $_.Exception.Message.Substring(0,120)) }
Write-Output "=== tpm ==="
Get-Tpm | Select-Object TpmPresent, TpmReady, TpmEnabled | Format-List | Out-String
Write-Output "=== VBS/hypervisor ==="
Get-CimInstance Win32_DeviceGuard -ErrorAction SilentlyContinue |
    Select-Object VirtualizationBasedSecurityStatus, SecurityServicesRunning | Format-List | Out-String
Write-Output "FW_DONE"
