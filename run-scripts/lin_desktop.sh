#!/bin/bash
# Real-desktop run: installer on :0 (kwin + compositor), screenshot result.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
timeout 10 bash -c 'DISPLAY=:0 xdpyinfo >/dev/null' || { echo DESKTOP_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-desktop.out 2>&1 < /dev/null &
for i in $(seq 1 9); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*10))s avail=${avail}MB"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
xwd -display :0 -root -silent 2>/dev/null | convert xwd:- /tmp/cc-desktop.png && ls -la /tmp/cc-desktop.png
echo "=== verdict ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "alerts=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -a -c 'not supported')"
echo DESKTOP_DONE
