#!/bin/bash
# Drive the Creative Cloud installer on a headless X server and capture visual
# evidence, which the Wayland session could not provide.
#
# XAUTHORITY is not needed for Xvfb (no auth cookie), and xwd works against it,
# so progress can be both logged and photographed.
set -u
export DISPLAY=:99
unset XAUTHORITY
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export WINEDEBUG=-all
WINE=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

OUT=/tmp/cc-headless.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
SHOTS=/home/kubuntu/adobe-wine-lab/runs/cc-headless
SECS=${1:-300}

mkdir -p "$SHOTS"
pkill -9 -f 'Set-Up' 2>/dev/null
pkill -9 -f 'msedgewebview2' 2>/dev/null
sleep 3
rm -f "$OUT" "$LOG"
rm -f "$SHOTS"/*.png

cd /home/kubuntu/Downloads
setsid "$WINE" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &

for i in $(seq 1 $((SECS / 30))); do
  sleep 30
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  rs=$(grep -c 'onWindowResize' "$LOG" 2>/dev/null || echo 0)
  st=$(grep -oE 'state: [A-Z_]+' "$LOG" 2>/dev/null | tail -1)
  mem=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*30))s log=$n webview2=$wv resize=$rs mem_avail=${mem}MB last=${st:-none}"
  # photograph the root window each interval; cheap and shows whether anything drew
  xwd -root -silent 2>/dev/null | convert xwd:- "$SHOTS/t$((i*30)).png" 2>/dev/null || true
done

echo
echo "=== workflow states (oracle reaches 7) ==="
grep -oE 'state: [A-Z_]+' "$LOG" 2>/dev/null | sort -u
echo "(end)"
echo
echo "=== onWindowResize ==="
grep -a 'onWindowResize' "$LOG" 2>/dev/null | head -2 || echo "(absent)"
echo
echo "=== screenshots ==="
ls -la "$SHOTS"/*.png 2>/dev/null | tail -5
echo
echo "=== non-fixme capture tail ==="
grep -avE '^[0-9a-f]+:fixme|WARNING: dzn|libEGL|^$' "$OUT" | tail -15
echo HEADLESS_DONE
