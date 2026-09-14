#!/bin/bash
# Fatality trap: attach to a browser child, resume through exceptions, and
# report whether the process SURVIVES (caught exception) or DIES (exit code).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-fat.out 2>&1 < /dev/null &
echo "launched"

for i in $(seq 1 24); do
  sleep 5
  n=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  if [ "$n" -ge "3" ]; then echo "children=$n"; break; fi
done
sleep 5
WPIDS=$("$W" tasklist 2>/dev/null | grep -a -i 'msedgewebview2' | awk '{print $2}' | tr '\n' ' ')
echo "wpids=$WPIDS"
VICTIM=$(echo "$WPIDS" | awk '{print $1}')
echo "victim_wpid=$VICTIM"

( printf 'handle SIGTRAP stop print nopass\ncont\nbt 8\ncont\nbt 8\ncont\nbt 8\ncont\ndetach\nquit\n'; sleep 200 ) \
  | timeout 220 "$W" winedbg --gdb "$VICTIM" > /tmp/gdb-fat.out 2>&1
echo "gdb rc=$?"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
sleep 1
echo "=== trap log ==="
grep -a -E 'SIGTRAP|#[0-9]|exited|exit code|Detaching|Kill|killed|Terminated|program exited|Inferior' /tmp/gdb-fat.out | head -40
echo FAT_DONE
