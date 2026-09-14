#!/bin/bash
# Full Download-button experiment: launch, wait for error dialog, click
# Download, capture network + screen.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-dlclick.out 2>&1 < /dev/null &
echo "launched, waiting for error dialog..."
for i in $(seq 1 24); do
  sleep 10
  if tr -d '\000' < "$LOG" 2>/dev/null | grep -a -q 'not supported'; then
    echo "DIALOG_UP after ~$((i*10))s"
    break
  fi
  avail=$(free -m | awk '/^Mem:/{print $7}')
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; exit 1; fi
done
sleep 20
echo "=== conns before click ==="
ss -tn 2>/dev/null | wc -l
echo "=== clicking Download ==="
xdotool mousemove 820 543 click 1 2>&1
sleep 15
echo "=== conns after click ==="
ss -tnp 2>/dev/null | head -12
echo "=== Downloads dir ==="
ls -lat ~/Downloads/ | head -5
xwd -root -silent 2>/dev/null | convert xwd:- /tmp/afterdl.png 2>/dev/null && echo SHOT_OK
echo DLCLICK_DONE
