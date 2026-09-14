#!/bin/bash
# H3 run phase only: identity/swap/Xvfb already in place. Every blocking
# call has a timeout. 10s guard granularity, kills installer under 400MB.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

pkill -9 -f 'lin_h3_run[.]sh' 2>/dev/null || true

OUT=/tmp/cc-h3.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
SHOTS=/home/kubuntu/adobe-wine-lab/runs/cc-h3
mkdir -p "$SHOTS"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f "$OUT" "$LOG"; rm -f "$SHOTS"/*.png

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
SECS=${1:-150}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*10))s log=$n wv2=$wv avail=${avail}MB"
  if [ "$avail" -lt 400 ]; then
    echo "MEMORY GUARD: avail ${avail}MB < 400MB, killing installer"
    pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
    sleep 2
    pkill -9 -f 'msedgewebview2' 2>/dev/null
    echo "GUARD_TRIPPED"
    break
  fi
done
timeout 10 bash -c 'xwd -root -silent | convert xwd:- "$0/final.png"' "$SHOTS" 2>/dev/null || true

echo "=== not-supported? ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -a 'not supported' | cut -c1-160 | head -2 || echo "(absent)"
echo "=== workflow states ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "(end)"
echo "=== onWindowResize ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -ac 'onWindowResize'
free -m | head -2
echo H3_DONE
