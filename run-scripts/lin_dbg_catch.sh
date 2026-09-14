#!/bin/bash
# Catch a crashing Chromium child under the winedbg gdb-proxy and dump the
# fault context (backtrace + registers + stack words). The int3 CHECK text
# often sits on the crashing thread's stack, which minidumps omit.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
PROXY_PORT=12345

pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-dbg.out 2>&1 < /dev/null &
echo "installer launched, waiting for browser children..."

# Wait for msedgewebview2 unix pids to appear (up to 90s).
CHILD=""
for i in $(seq 1 18); do
  sleep 5
  CHILD=$(pgrep -f 'msedgewebview2' | head -1)
  if [ -n "$CHILD" ]; then echo "child unix pid=$CHILD"; break; fi
done
if [ -z "$CHILD" ]; then echo "NO_CHILDREN"; exit 1; fi

# Give it a moment to get past startup crashes into the steady GPU crash.
sleep 10
# Pick the youngest (most recently spawned, likely a fresh GPU/renderer).
CHILD=$(pgrep -f 'msedgewebview2' | tail -1)
echo "attaching to unix pid=$CHILD"

# winedbg needs the Windows pid; list to correlate.
"$W" winedbg --gdb "$CHILD" > /tmp/gdbproxy.log 2>&1 &
PROXY=$!
sleep 4
cat /tmp/gdbproxy.log | head -5

echo "quit" | timeout 10 nc 127.0.0.1 $PROXY_PORT 2>/dev/null | head -2 || echo "(proxy port check done)"
kill $PROXY 2>/dev/null
echo CATCH_DONE
