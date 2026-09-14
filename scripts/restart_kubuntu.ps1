Restart-VM -Name 'Kubuntu-26.04' -Force
Start-Sleep -Seconds 10
Get-VM -Name 'Kubuntu-26.04' | Select-Object Name, State, Uptime | Out-File C:\Temp\vmrestart.txt
