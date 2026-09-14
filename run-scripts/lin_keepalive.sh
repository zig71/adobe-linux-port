#!/bin/bash
# Keepalive launcher: fresh installer on the DESKTOP session (:0) for
# interactive sign-in via the VM console. No kills, monitor only.
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export WINEDEBUG=-all
unset WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

lab_x_sanity
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG" /tmp/cc-keepalive.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-keepalive.out 2>&1 < /dev/null &
echo "LAUNCHED display=$DISPLAY"
for i in $(seq 1 30); do
  sleep 10
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  st=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | tail -1)
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  echo "t=$((i*10))s log=$n wv2=$wv last=${st:-none}"
  case "$st" in *START_SIGNIN_WORKFLOW*) echo "SIGNIN_REACHED"; break;; esac
done
echo "=== windows on desktop ==="
DISPLAY=:0 wmctrl -l 2>/dev/null | head -6 || DISPLAY="$DISPLAY" wmctrl -l 2>/dev/null | head -6
echo KEEPALIVE_DONE
