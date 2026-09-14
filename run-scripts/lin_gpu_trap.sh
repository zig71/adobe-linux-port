#!/bin/bash
# GPU trap: attach to a --type=gpu-process child specifically and catch
# its int3 with stack (holds the CHECK text). Uses wmic-style cmdlines
# via tasklist + pgrep correlation is unreliable, so: enumerate unix pids
# with full cmdlines, pick the gpu-process one, map to wpid via wine tasklist.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-gpu.out 2>&1 < /dev/null &
echo "launched, waiting for gpu-process child..."

GPU_UNIX=""
for i in $(seq 1 30); do
  sleep 5
  GPU_UNIX=$(pgrep -af 'type=gpu-process' | head -1 | awk '{print $1}')
  if [ -n "$GPU_UNIX" ]; then echo "gpu_unix=$GPU_UNIX"; break; fi
done
if [ -z "$GPU_UNIX" ]; then echo "NO_GPU_CHILD"; exit 1; fi
sleep 3

# Map: tasklist lines contain wpid; correlate by start order is fragile.
# Instead list all and let winedbg attach by trying each msedge wpid until
# the trap fires: here just take all wpids, trap the LAST (youngest).
WPIDS=$("$W" tasklist 2>/dev/null | grep -a -i 'msedgewebview2' | awk '{print $2}' | tr '\n' ' ')
echo "wpids=$WPIDS"
VICTIM=""
for w in $WPIDS; do VICTIM=$w; done
echo "victim_wpid=$VICTIM (youngest)"

( printf 'handle SIGTRAP stop print nopass\ncont\nbt 12\nx/64gx $rsp\ncont\nbt 8\ncont\ndetach\nquit\n'; sleep 200 ) \
  | timeout 220 "$W" winedbg --gdb "$VICTIM" > /tmp/gdb-gpu.out 2>&1
echo "gdb rc=$?"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== trap log ==="
grep -a -E 'SIGTRAP|#[0-9]|rip |0x[0-9a-f]+:\t|exited|Inferior|Detaching' /tmp/gdb-gpu.out | head -60
echo GPUTRAP_DONE
