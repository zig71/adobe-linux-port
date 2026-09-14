#!/bin/bash
# Version-gate bisection: run browser under win8 and watch crash rate.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
"$W" reg add 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe' /v Version /d "$1" /f 2>&1 | tail -1
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
R="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Crashpad/reports"
rm -f "$R"/*.dmp 2>/dev/null

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-vergate.out 2>&1 < /dev/null &
for i in $(seq 1 18); do
  sleep 5
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  dmps=$(ls "$R"/*.dmp 2>/dev/null | wc -l)
  echo "t=$((i*5))s wv2=$wv dumps=$dmps"
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== dumps by time ==="
ls -l --time-style=+%M:%S "$R"/*.dmp 2>/dev/null | awk '{print $6}' | head -8
echo VERGATE_DONE
