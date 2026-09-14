#!/bin/bash
# H1 experiment: does the requirements gate key on build number?
# Changes ONLY CurrentBuild/CurrentBuildNumber to the oracle's 26200,
# then a short memory-guarded headless run. Reads states reached.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

echo "=== swap for OOM headroom ==="
if ! swapon --show 2>/dev/null | grep -q '/swapfile'; then
  echo ${LAB_PW} | sudo -S -p '' fallocate -l 4G /swapfile 2>/dev/null \
    || echo ${LAB_PW} | sudo -S -p '' dd if=/dev/zero of=/swapfile bs=1M count=4096 status=none
  echo ${LAB_PW} | sudo -S -p '' chmod 600 /swapfile
  echo ${LAB_PW} | sudo -S -p '' mkswap /swapfile >/dev/null 2>&1
  echo ${LAB_PW} | sudo -S -p '' swapon /swapfile
fi
swapon --show | head -3
free -m | head -2

echo
echo "=== Xvfb :99 ==="
pgrep -f 'Xvfb :99' >/dev/null || setsid Xvfb :99 -screen 0 1280x960x24 -nolisten tcp >/tmp/xvfb.log 2>&1 < /dev/null
sleep 3
DISPLAY=:99 xdpyinfo 2>&1 | grep dimensions || { echo XVFB_DEAD; tail -5 /tmp/xvfb.log; exit 1; }

echo
echo "=== H1: build -> 26200 (only) ==="
K='HKLM\Software\Microsoft\Windows NT\CurrentVersion'
"$W" reg add "$K" /v CurrentBuild /d 26200 /f 2>&1 | tail -1
"$W" reg add "$K" /v CurrentBuildNumber /d 26200 /f 2>&1 | tail -1

OUT=/tmp/cc-h1.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
SHOTS=/home/kubuntu/adobe-wine-lab/runs/cc-h1
mkdir -p "$SHOTS"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f "$OUT" "$LOG"; rm -f "$SHOTS"/*.png

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
SECS=${1:-120}
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
echo H1_DONE
