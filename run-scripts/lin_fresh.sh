#!/bin/bash
# Fresh-binary test: does the current Adobe build pass the gate?
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
unset WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-fresh.out 2>&1 < /dev/null &
SECS=${1:-240}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  st=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | tail -1)
  rs=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -ac 'onWindowResize')
  echo "t=$((i*10))s log=$n wv2=$wv avail=${avail}MB resize=$rs last=${st:-none}"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
xwd -root -silent 2>/dev/null | convert xwd:- /tmp/fresh-final.png 2>/dev/null || true
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== verdict ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aE 'Workflow start|showErrorAlert|not supported|state: ' | cut -c1-120 | head -14
echo FRESH_DONE
