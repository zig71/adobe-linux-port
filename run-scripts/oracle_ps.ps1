$ps = Get-Process | Where-Object { $_.ProcessName -like '*Creative*' -or $_.ProcessName -like '*Adobe Desktop*' -or $_.ProcessName -like '*CoreSync*' } | Select-Object -First 8
foreach ($p in $ps) {
  $c = (Get-CimInstance Win32_Process -Filter ("ProcessId=" + $p.Id)).CommandLine
  Write-Output ("PID=" + $p.Id + " NAME=" + $p.ProcessName)
  Write-Output ("  CMD=" + $c)
}
echo PROC_DONE
