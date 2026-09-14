$h = Select-String -Path C:\Users\winnie\AppData\Local\Temp\CreativeCloud\ACC\*.log -Pattern '@AdobeID' | Select-Object -Last 1
if ($h -eq $null) { Write-Output "NO_MATCH" } else { Write-Output $h.Line.Substring(0, [Math]::Min(300, $h.Line.Length)) }
echo FIND_DONE
