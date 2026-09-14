#!/bin/bash
# Warm-run test: launch WITHOUT killing anything first. If a warm browser
# is already up from a previous run, the gate may find it ready.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }

OUT=/tmp/cc-warm2.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
echo "pre-existing browser children: $(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)"
rm -f "$OUT"
# NOTE: no pkill, no LOG delete (second instance appends/creates anew).

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
SECS=${1:-150}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*10))s avail=${avail}MB"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
echo "=== verdict (all runs in log) ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aE 'showErrorAlert|not supported|state: ' | cut -c1-110 | tail -12
echo WARM2_DONE
