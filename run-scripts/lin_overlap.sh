#!/bin/bash
# Overlapped warm test in one sequence: warmer, +40s, warm2 (no kills).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-warmer.out 2>&1 < /dev/null &
echo "warmer launched, waiting 40s for its browser..."
sleep 40
echo "browser children now: $(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)"
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-warm2b.out 2>&1 < /dev/null &
echo "warm2 launched into warm browser"
for i in $(seq 1 15); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*10+40))s avail=${avail}MB"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== verdict (both runs) ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aE 'showErrorAlert|not supported|state: ' | cut -c1-110 | head -16
echo OVERLAP_DONE
