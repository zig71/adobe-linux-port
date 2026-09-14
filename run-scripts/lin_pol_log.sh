#!/bin/bash
# Policy-logging run: Chromium logs via registry policy (installer untouched).
# Goal: chrome_debug.log naming the crashing CHECK.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
unset WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }

"$W" regedit /s /tmp/webview2_logging.reg 2>&1 | tail -1

OUT=/tmp/cc-pol.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f "$OUT" "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
SECS=${1:-180}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  dbg=$(find "$WINEPREFIX/drive_c/users" -name 'chrome_debug.log' 2>/dev/null | head -1)
  echo "t=$((i*10))s log=$n avail=${avail}MB chromedebug=${dbg:-none}"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
sleep 1
echo "=== chrome_debug tail ==="
for f in $(find "$WINEPREFIX/drive_c/users" -name 'chrome_debug.log' 2>/dev/null | head -3); do
  echo "--- $f"
  grep -a -i -E 'check fail|fatal|error|gpu|angle|d3d|vulkan|fallback|crash' "$f" | tail -20
done
echo "=== WAM states ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "(end)"
echo POLLOG_DONE
