#!/bin/bash
# Fresh-binary gate check in prefix-wv2 (100s).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-fresh2.out 2>&1 < /dev/null &
for i in $(seq 1 10); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  wv2=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  echo "t=$((i*10))s wv2=$wv2 avail=${avail}MB"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
echo "=== verdict ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "alerts=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -a -c 'not supported')"
echo "resize=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -a -c 'onWindowResize')"
echo FRESH2_DONE
