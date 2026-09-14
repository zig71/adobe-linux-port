#!/bin/bash
# First-exception census: log every raised exception with code/address.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=+seh,+tid,+pid
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f /tmp/cc-seh.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-seh.out 2>&1 < /dev/null &
SECS=${1:-150}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*10))s avail=${avail}MB outkb=$(du -k /tmp/cc-seh.out 2>/dev/null | cut -f1)"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== exception census (code x count) ==="
grep -a -o -E 'code=c[0-9a-f]{7}' /tmp/cc-seh.out 2>/dev/null | sort | uniq -c | sort -rn | head -10
echo "=== first-chance details ==="
grep -a -E 'Exception|exception' /tmp/cc-seh.out 2>/dev/null | grep -a -v -E 'RaiseException|handle_exception' | cut -c1-150 | head -15
echo SEHCENSUS_DONE
