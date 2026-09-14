#!/bin/bash
# wait4 trap: read reaped exit codes at the Linux level (strace on installer).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG" /tmp/strace-wait.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-str.out 2>&1 < /dev/null &
sleep 6
IPID=$(pgrep -f 'Creative_Cloud_Set-Up' | head -1)
echo "installer_unix=$IPID"
if [ -z "$IPID" ]; then echo "NO_INSTALLER"; exit 1; fi
setsid nohup strace -f -e trace=wait4,waitpid,waitid -p "$IPID" -o /tmp/strace-wait.out > /dev/null 2>&1 < /dev/null &
echo "strace attached, waiting for gate..."
for i in $(seq 1 12); do
  sleep 10
  if tr -d '\000' < "$LOG" 2>/dev/null | grep -a -q 'showErrorAlert'; then
    echo "ALERT at ~$((i*10+6))s, waiting 5s more"
    sleep 5
    break
  fi
done
pkill -9 -f 'strac[e]' 2>/dev/null
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== reaped exit statuses ==="
grep -a -E 'wait4?\(|waitpid' /tmp/strace-wait.out 2>/dev/null | grep -a -v -E 'resumed|unfinished' | head -20
echo STRACE_DONE
