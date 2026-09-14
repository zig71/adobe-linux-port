$s = (Get-ChildItem -Recurse C:\Users\winnie\AppData\LocalLow\Adobe\GrowthSDK -ErrorAction SilentlyContinue | Measure-Object -Property Length -Sum).Sum
Write-Output ("GROWTH_BYTES=" + $s)
echo SIZE_DONE
