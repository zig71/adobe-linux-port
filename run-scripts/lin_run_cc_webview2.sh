#!/bin/bash
# Run the real Creative Cloud installer in the prefix where the WebView2
# runtime is staged and registered, and watch for evidence that it takes its
# WebView2 path rather than the IE fallback.
#
# Markers, in increasing order of confidence:
#   - %LOCALAPPDATA%/Adobe/webview2/... user-data directory (Windows creates this)
#   - an msedgewebview2 process
#   - ViewMediatorUIWin | onWindowResize in WAM.log
#   - WorkflowManager states
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-wv2-run.out 2>&1 < /dev/null &

for i in 1 2 3 4 5 6; do
  sleep 40
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  rs=$(grep -c 'onWindowResize' "$LOG" 2>/dev/null || echo 0)
  st=$(grep -oE 'state: [A-Z_]+' "$LOG" 2>/dev/null | tail -1)
  wv=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  echo "t=$((i*40))s loglines=$n onWindowResize=$rs webview2_procs=$wv last_state=${st:-none}"
done

echo
echo "=== webview2 user-data directory (Adobe's own) ==="
find "$WINEPREFIX/drive_c/users" -ipath '*Adobe*webview2*' -maxdepth 9 2>/dev/null | head -8
echo "(end)"
echo
echo "=== any EBWebView dir anywhere in the prefix ==="
find "$WINEPREFIX/drive_c" -ipath '*EBWebView*' -maxdepth 10 -type d 2>/dev/null | head -8
echo "(end)"
echo
echo "=== workflow states reached ==="
grep -oE 'state: [A-Z_]+' "$LOG" 2>/dev/null | sort -u
echo "(end)"
echo
echo "=== mshtml fallback still taken? ==="
grep -ac 'ActiveScriptSite' /tmp/cc-wv2-run.out 2>/dev/null || echo 0
echo
echo "=== installer errors of interest ==="
grep -aiE 'webview2|msedge|dcomp|not implemented|E_NOTIMPL' /tmp/cc-wv2-run.out 2>/dev/null | head -10
echo RUN_DONE
