#!/bin/bash
# Capture the registry reads in the 18ms window between
# CHECK_GENERAL_SYSTEM_REQUIREMENTS and showErrorAlert(21).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=+wbemprox,+tid,+pid
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }

OUT=/tmp/cc-reg.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f "$OUT" "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
sleep 6
pgrep -f 'Set-Up' | head -3 > /tmp/cc-installer.pids
echo "installer_pids=$(cat /tmp/cc-installer.pids | tr '\n' ' ')"
SECS=${1:-75}
for i in $(seq 1 $((SECS / 15))); do
  sleep 15
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*15))s avail=${avail}MB outkb=$(du -k "$OUT" 2>/dev/null | cut -f1)"
  if grep -a -q 'showErrorAlert' "$LOG" 2>/dev/null; then
    echo "ALERT_LOGGED, waiting 10s more then killing"
    sleep 10
    break
  fi
  if [ "$avail" -lt 400 ]; then
    echo "MEMORY GUARD TRIPPED"
    break
  fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
sleep 1
pkill -9 -f 'msedgewebview2' 2>/dev/null
echo "=== WAM verdict ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aE 'showErrorAlert|not supported|state: ' | cut -c1-140 | head -8
echo "=== out size ==="
ls -la "$OUT"
echo REGTRACE_DONE
