#!/bin/bash
# DevTools UA capture: what OS does the browser report?
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
export WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS="--remote-debugging-port=9222"
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-devtools.out 2>&1 < /dev/null &
for i in $(seq 1 12); do
  sleep 10
  R=$(curl -s -m 5 http://127.0.0.1:9222/json/version 2>/dev/null || echo "")
  if [ -n "$R" ]; then
    echo "DEVTOOLS_UP at ~$((i*10))s"
    echo "$R" | head -20
    break
  fi
  echo "t=$((i*10))s waiting for devtools..."
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo DEVTOOLS_DONE
