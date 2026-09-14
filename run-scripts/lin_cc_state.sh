#!/bin/bash
# Report how far the Creative Cloud installer gets now that the WebView2 runtime
# is found and Chromium is actually launching.
#
# Writes the diagnosis to a file so the caller does not have to escape it.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
RUN=/tmp/cc-state.out

echo "=== WAM.log ==="
if [ -f "$LOG" ]; then
  echo "lines=$(wc -l < "$LOG")"
  echo "--- workflow states reached ---"
  grep -oE 'state: [A-Z_]+' "$LOG" | sort -u
  echo "--- onWindowResize count ---"
  grep -c 'onWindowResize' "$LOG" || true
  echo "--- last 6 semantic lines ---"
  tr -d '\000' < "$LOG" | grep -aE '\[(INFO|WARN|ERROR)\]' | tail -6 | cut -c1-170
else
  echo "no WAM.log at $LOG"
fi

echo
echo "=== Chromium's own log, if it wrote one ==="
find "$WINEPREFIX/drive_c/users" -ipath '*EBWebView*' \
     \( -iname 'chrome_debug.log' -o -iname '*.log' \) 2>/dev/null | head -5
echo "(end)"

echo
echo "=== Chromium profile created (parity check vs Windows) ==="
ls -1 "$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/" 2>/dev/null | head -40
echo "(end)"

echo
echo "=== processes alive ==="
pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0
pgrep -c -f 'Set-Up' 2>/dev/null || echo 0

echo
echo "=== crash dumps ==="
find "$WINEPREFIX/drive_c/users" -ipath '*EBWebView*' -iname '*.dmp' 2>/dev/null | head -5
echo "(end)"
echo STATE_DONE
