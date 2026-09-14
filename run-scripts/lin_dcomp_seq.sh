#!/bin/bash
# dcomp-sequence run: log OUR dcomp implementation's calls to see what
# Chromium drives before the int3 (no win7 lie; truthful version).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=+dcomp,+tid,+pid
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f /tmp/cc-dcomp.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-dcomp.out 2>&1 < /dev/null &
SECS=${1:-150}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  echo "t=$((i*10))s wv2=$wv avail=${avail}MB outkb=$(du -k /tmp/cc-dcomp.out 2>/dev/null | cut -f1)"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== dcomp call summary (browser pids only) ==="
grep -a -E 'trace:dcomp' /tmp/cc-dcomp.out 2>/dev/null | grep -a -o -E '[A-Za-z0-9_]+\\(' | sort | uniq -c | sort -rn | head -30
echo "(end summary)"
echo DCOMPSEQ_DONE
