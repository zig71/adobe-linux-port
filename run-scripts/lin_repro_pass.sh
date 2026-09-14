#!/bin/bash
# Reproduce the passing recipe in staging (100s gate check).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-staging
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
P=/home/kubuntu/adobe-wine-lab/prefix-staging/drive_c/windows
for f in system32/d3d12.dll system32/d3d12core.dll syswow64/d3d12.dll syswow64/d3d12core.dll; do
  [ -f "$P/$f" ] && mv "$P/$f" "$P/$f.nope" && echo "hid $f"
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-repro.out 2>&1 < /dev/null &
for i in $(seq 1 10); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*10))s avail=${avail}MB"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
for f in system32/d3d12.dll system32/d3d12core.dll syswow64/d3d12.dll syswow64/d3d12core.dll; do
  [ -f "$P/$f.nope" ] && mv "$P/$f.nope" "$P/$f" && echo "restored $f"
done
echo "=== verdict ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "alerts=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -a -c 'not supported')"
echo REPRO_DONE
