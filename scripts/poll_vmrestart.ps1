$t = 0
while ($t -lt 100 -and -not (Test-Path C:\Temp\vmrestart.txt)) { Start-Sleep -Seconds 5; $t += 5 }
if (Test-Path C:\Temp\vmrestart.txt) { Get-Content C:\Temp\vmrestart.txt } else { echo NO_RESULT_YET }
