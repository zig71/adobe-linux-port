#!/bin/bash
# Multi-trap: attach to up to 4 browser children in parallel; whoever hits
# the int3 first gets a 300-qword stack dump (holds the CHECK text).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-multi.out 2>&1 < /dev/null &
echo "launched"

for i in $(seq 1 24); do
  sleep 5
  n=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  if [ "$n" -ge "4" ]; then echo "children=$n"; break; fi
done
sleep 10
WPIDS=$("$W" tasklist 2>/dev/null | grep -a -i 'msedgewebview2' | awk '{print $2}' | head -4 | tr '\n' ' ')
echo "victims=$WPIDS"
k=0
for w in $WPIDS; do
  k=$((k + 1))
  ( printf 'handle SIGTRAP stop print nopass\ncont\nbt 10\nx/300gx $rsp\ncont\ndetach\nquit\n'; sleep 200 ) \
    | timeout 220 "$W" winedbg --gdb "$w" > /tmp/gdb-multi$k.out 2>&1 &
done
wait
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== trap hits ==="
grep -a -l 'SIGTRAP.*[Tt]rap\|#[0-9]' /tmp/gdb-multi*.out 2>/dev/null
for f in /tmp/gdb-multi1.out /tmp/gdb-multi2.out /tmp/gdb-multi3.out /tmp/gdb-multi4.out; do
  echo "--- $f"
  grep -a -E '#[0-9] |SIGTRAP|oneauth|msedge' "$f" 2>/dev/null | head -12
done
echo MULTITRAP_DONE
