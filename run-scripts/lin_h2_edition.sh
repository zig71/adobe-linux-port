#!/bin/bash
# H2 experiment: does the requirements gate key on edition identity?
# Sets EditionId=Core + ProductName='Windows 10 Home' (oracle exact),
# keeps build 26200 from H1. Short memory-guarded headless run.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

echo "=== memory before ==="
free -m | head -2
pgrep -f 'Xvfb :99' >/dev/null || setsid Xvfb :99 -screen 0 1280x960x24 -nolisten tcp >/tmp/xvfb.log 2>&1 < /dev/null
sleep 2

echo
echo "=== H2: edition -> Core/Home (oracle exact) ==="
K='HKLM\Software\Microsoft\Windows NT\CurrentVersion'
"$W" reg add "$K" /v EditionId /d Core /f 2>&1 | tail -1
"$W" reg add "$K" /v ProductName /d 'Windows 10 Home' /f 2>&1 | tail -1
"$W" reg query "$K" 2>/dev/null | grep -E 'CurrentBuild|ProductName|Edition|UBR|Display'

OUT=/tmp/cc-h2.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
SHOTS=/home/kubuntu/adobe-wine-lab/runs/cc-h2
mkdir -p "$SHOTS"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f "$OUT" "$LOG"; rm -f "$SHOTS"/*.png

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
SECS=${1:-150}
for i in $(seq 1 $((SECS / 30))); do
  sleep 30
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  avail=$(free -m | awk '/^Mem:/{print $7}')
  swapused=$(free -m | awk '/^Swap:/{print $3}')
  echo "t=$((i*30))s log=$n wv2=$wv avail=${avail}MB swapused=${swapused}MB"
  xwd -root -silent 2>/dev/null | convert xwd:- "$SHOTS/t$((i*30)).png" 2>/dev/null || true
  if [ "$avail" -lt 250 ]; then
    echo "MEMORY GUARD: avail ${avail}MB < 250MB, killing installer to protect guest"
    pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
    echo "GUARD_TRIPPED"
    break
  fi
done

echo
echo "=== not-supported? ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -a 'not supported' | cut -c1-160 | head -2 || echo "(absent)"
echo "=== workflow states ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "(end)"
echo H2_DONE
