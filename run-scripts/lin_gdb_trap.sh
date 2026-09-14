#!/bin/bash
# Trap a crashing Chromium child: attach via winedbg gdb-proxy, continue,
# and on the int3 CHECK dump backtrace + registers + stack (holds the
# CHECK text minidumps omit). Piped gdb commands with a sleep-held stdin.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }

pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f /tmp/cc-dbg.out /tmp/gdb-catch.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-dbg.out 2>&1 < /dev/null &
echo "installer launched"

# Wait for several browser children (up to 120s).
for i in $(seq 1 24); do
  sleep 5
  n=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  if [ "$n" -ge "3" ]; then echo "children=$n"; break; fi
done

# Windows pids of the browser children.
WPIDS=$("$W" tasklist 2>/dev/null | grep -a -i 'msedgewebview2' | awk '{print $2}' | head -5 | tr '\n' ' ')
echo "wpids=$WPIDS"
VICTIM=$(echo "$WPIDS" | awk '{print $2}')
if [ -z "$VICTIM" ]; then VICTIM=$(echo "$WPIDS" | awk '{print $1}'); fi
if [ -z "$VICTIM" ]; then echo "NO_VICTIM"; exit 1; fi
echo "victim_wpid=$VICTIM"

( printf 'handle SIGTRAP stop print nopass\ncont\nbt 12\nx/220gx $rsp\ncont\ndetach\nquit\n'; sleep 200 ) \
  | timeout 220 "$W" winedbg --gdb "$VICTIM" > /tmp/gdb-catch.out 2>&1
echo "gdb rc=$?"

pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
sleep 1
echo "=== trap log ==="
grep -a -E 'SIGTRAP|#[0-9]|rip |0x[0-9a-f]+:[[:space:]]' /tmp/gdb-catch.out | head -120
echo CATCH_DONE
