#!/bin/bash
# Chromium file-logging run: explicit --log-file (env flags proven safe).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
export WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS="--enable-logging --log-file=C:\\users\\kubuntu\\AppData\\Local\\Temp\\chrome_debug.log --v=1"

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-logfile.out 2>&1 < /dev/null &
SECS=${1:-180}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  clog=$(find "$WINEPREFIX/drive_c" -name 'chrome_debug.log' 2>/dev/null | head -1)
  echo "t=$((i*10))s log=$n avail=${avail}MB chromelog=${clog:-none}"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== chrome log CHECK/FATAL ==="
for f in $(find "$WINEPREFIX/drive_c" -name 'chrome_debug.log' 2>/dev/null | head -2); do
  echo "--- $f"
  grep -a -i -E 'check fail|fatal|gpu.*(crash|fallback|init fail)|angle.*(fail|error)|d3d.*(fail|error)|vulkan.*(fail|error)|sandbox.*(fail|error)' "$f" | head -20
done
echo LOGFILE_DONE
