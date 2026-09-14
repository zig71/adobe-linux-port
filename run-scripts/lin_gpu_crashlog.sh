#!/bin/bash
# Chromium-logging run: capture WHY the GPU/renderer processes crash.
# WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS is the supported WebView2 way to
# pass Chromium flags without touching the application.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
export WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS="--enable-logging=stderr --v=1"
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }

OUT=/tmp/cc-crash.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f "$OUT" "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
SECS=${1:-150}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  echo "t=$((i*10))s log=$n avail=${avail}MB outkb=$(du -k "$OUT" 2>/dev/null | cut -f1)"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
sleep 1
echo "=== GPU process verdicts ==="
grep -a -i -E 'gpu process|GPU.*(crash|fail|fallback|usable|goodbye)|angle|d3d11|vulkan|swiftshader|sandbox' "$OUT" | head -25
echo "(end)"
echo "=== chrome_debug.log ==="
find "$WINEPREFIX/drive_c/users" -name 'chrome_debug.log' 2>/dev/null | head -3
echo CRASHLOG_DONE
