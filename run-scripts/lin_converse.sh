#!/bin/bash
# Converse test: kill all browser children BEFORE the gate (t+5s).
# If the gate still fails identically, it does not depend on live children.
# If behavior changes (no check / different error), children presence matters.
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
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-conv.out 2>&1 < /dev/null &
sleep 5
echo "t=5s killing all browser children"
pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0
pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 2
pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0
for i in $(seq 1 11); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  echo "t=$((i*10+7))s log=$n avail=${avail}MB"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== verdict ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aE 'showErrorAlert|not supported|state: ' | cut -c1-120 | head -10
echo CONVERSE_DONE
