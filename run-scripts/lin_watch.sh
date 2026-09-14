#!/bin/bash
# Death-watch: 5s-granularity browser census + dump timestamps.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
R="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Crashpad/reports"

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f "$R"/*.dmp 2>/dev/null

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-watch.out 2>&1 < /dev/null &
for i in $(seq 1 24); do
  sleep 5
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  dmps=$(ls "$R"/*.dmp 2>/dev/null | wc -l)
  echo "t=$((i*5))s wv2=$wv dumps=$dmps"
  if [ "$wv" = "0" ] && [ "$i" -gt "6" ]; then echo "ALL_DEAD"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== dump mtimes ==="
ls -l --time-style=+%H:%M:%S "$R"/*.dmp 2>/dev/null | awk '{print $6, $7}'
echo WATCH_DONE
