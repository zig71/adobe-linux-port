#!/bin/bash
# SwiftShader run: force software ANGLE, watch GPU survival + gate.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
export WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS="--use-angle=swiftshader"
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-sw.out 2>&1 < /dev/null &
SECS=${1:-180}
for i in $(seq 1 $((SECS / 5))); do
  sleep 5
  avail=$(free -m | awk '/^Mem:/{print $7}')
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  echo "t=$((i*5))s log=$n wv2=$wv avail=${avail}MB"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== verdict ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aE 'showErrorAlert|not supported|state: ' | cut -c1-110 | head -8
echo SWDONE
