#!/bin/bash
# SEH census + mid-run tasklist snapshots to identify throwing processes.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=+seh,+tid,+pid
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f /tmp/cc-seh2.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-seh2.out 2>&1 < /dev/null &
for i in 1 2 3 4 5 6; do
  sleep 20
  "$W" tasklist 2>/dev/null | grep -a -i -E 'Set-Up|msedgewebview2' | awk '{print $1, $2}' > /tmp/tasklist-t$((i*20)).txt
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*20))s avail=${avail}MB procs=$(wc -l < /tmp/tasklist-t$((i*20)).txt)"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== e06d7363 by pid ==="
grep -a -o -E '^[0-9a-f]+:[0-9a-f]+:trace:seh:dispatch_exception code=e06d7363' /tmp/cc-seh2.out 2>/dev/null | sort | uniq -c | head -12
echo "=== int3 by pid ==="
grep -a -o -E '^[0-9a-f]+:[0-9a-f]+:trace:seh:dispatch_exception code=80000003' /tmp/cc-seh2.out 2>/dev/null | sort | uniq -c | head -12
echo SEHID_DONE
