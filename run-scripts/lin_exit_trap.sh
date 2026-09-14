#!/bin/bash
# Exit-code trap: what codes does the gate read from reaped children?
# Attaches to the long-lived installer (stable target) and prints every
# GetExitCodeProcess *lpExitCode, auto-continuing.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-exitcode.out 2>&1 < /dev/null &
echo "launched, waiting for installer..."
sleep 8
IWPid=$("$W" tasklist 2>/dev/null | grep -a -i 'Creative_Cloud_Set-Up' | awk '{print $2}' | head -1)
if [ -z "$IWPid" ]; then echo "NO_INSTALLER"; exit 1; fi
( printf 'break GetExitCodeProcess\ncommands\nsilent\nset $ec = $rdx\nfinish\nprintf "EXITCODE handle=%%p code=%%d (0x%%x)\\n", $rdi, *(int*)$ec, *(int*)$ec\ncont\nend\ncont\ndetach\nquit\n'; sleep 90 ) \
  | timeout 110 "$W" winedbg --gdb "$IWPid" > /tmp/gdb-exitcode.out 2>&1
echo "gdb rc=$?"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== exit codes seen by the gate ==="
grep -a -E 'EXITCODE|Breakpoint' /tmp/gdb-exitcode.out | head -20
echo EXITTRAP_DONE
