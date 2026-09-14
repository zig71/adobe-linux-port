#!/bin/bash
# Child mortality census: do startup browser children survive?
# Samples browser unix pids twice; reports deaths. No debugger, no flags.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-census.out 2>&1 < /dev/null &
sleep 12
echo "=== pids at t+12s ==="
pgrep -af 'msedgewebview[2]' | head -15 | cut -c1-100
pgrep -f 'msedgewebview[2]' | sort -n > /tmp/pids12.txt
wc -l < /tmp/pids12.txt
sleep 8
echo "=== deaths by t+20s ==="
while read -r p; do
  if ! kill -0 "$p" 2>/dev/null; then echo "DIED pid=$p"; fi
done < /tmp/pids12.txt
echo "=== survivors ==="
pgrep -c -f 'msedgewebview[2]' || echo 0
echo "=== crashpad reports total ==="
ls ~/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Crashpad/reports/ 2>/dev/null | wc -l
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
echo CENSUS_DONE
