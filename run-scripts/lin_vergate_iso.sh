#!/bin/bash
# Version bisection in the isolated staging prefix (desktop session untouched).
# Usage: lin_vergate_iso.sh <winver>
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-staging
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
"$W" reg add 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe' /v Version /d "$1" /f 2>&1 | tail -1
R="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Crashpad/reports"
rm -f "$R"/*.dmp 2>/dev/null

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-iso.out 2>&1 < /dev/null &
for i in $(seq 1 18); do
  sleep 5
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  dmps=$(ls "$R"/*.dmp 2>/dev/null | wc -l)
  echo "t=$((i*5))s wv2=$wv dumps=$dmps"
done
echo VERGATE_ISO_DONE
