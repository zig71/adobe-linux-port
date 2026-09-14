#!/bin/bash
# Installer run on the working graphics path (llvmpipe Vulkan for DXVK).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
OUT=/tmp/cc-vk.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$OUT" "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
SECS=${1:-240}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  st=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | tail -1)
  rs=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -ac 'onWindowResize')
  echo "t=$((i*10))s log=$n wv2=$wv avail=${avail}MB resize=$rs last=${st:-none}"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
xwd -root -silent 2>/dev/null | convert xwd:- /tmp/vk-final.png 2>/dev/null || true
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== not-supported? ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -a 'not supported' | cut -c1-120 | head -2 || echo "(absent)"
echo "=== workflow states ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "(end)"
echo VKRUN_DONE
