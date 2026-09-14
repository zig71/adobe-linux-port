$h = Select-String -Path C:\Users\winnie\AppData\Local\Temp\CreativeCloud\ACC\ACC.log -Pattern '21:3[56]:' | Select-Object -First 40
foreach ($m in $h) {
  $l = $m.Line
  if ($l -match 'state|State|show|Show|view|View|window|Window|ready|Ready|navigat|Navigat') { Write-Output $l.Substring(0, [Math]::Min(200, $l.Length)) }
}
echo SIG_DONE
