#!/bin/bash
# Get Chromium's own diagnosis for the WebView2 browser process under Wine.
#
# WebView2 forwards WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS to the browser process,
# which is the supported way to turn on Chromium logging without touching the
# application. Everything the installer and Chromium print goes to the capture
# file, unfiltered.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-wv2}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
OUT=/tmp/cc-chromium.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"

# Turn on Chromium's verbose logging and send it to stderr.
export WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS="--enable-logging=stderr --v=1"

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$OUT" "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &

for i in 1 2 3 4 5; do
  sleep 35
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  rs=$(grep -c 'onWindowResize' "$LOG" 2>/dev/null || echo 0)
  echo "t=$((i*35))s installer_log=$n webview2_procs=$wv onWindowResize=$rs"
done

echo
echo "=== Chromium / installer messages ==="
wc -l < "$OUT"
grep -aiE 'chromium|ERROR|FATAL|CHECK failed|sandbox|mojo|gpu|vulkan|d3d' "$OUT" | head -30
echo "(end)"
echo
echo "=== last 25 lines of capture ==="
tail -25 "$OUT"
echo CHROMIUM_DONE
