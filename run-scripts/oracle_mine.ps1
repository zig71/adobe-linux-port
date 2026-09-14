$hits = Select-String -Path C:\AdobeCap\cc-1.pml -Pattern 'Creative Cloud\.exe.{0,100}' -AllMatches | Select-Object -First 5
foreach ($h in $hits) { foreach ($m in $h.Matches) { $m.Value } }
echo MINE_DONE
