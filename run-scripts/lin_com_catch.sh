#!/bin/bash
# CoCreateInstance catch: which COM class does OneAuth need?
# Relay filtered live to COM activation only; browser left alive.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=+ole,+tid,+pid,+timestamp

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }

OUT=/tmp/cc-com.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f "$OUT" "$LOG"

W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe 2>&1 < /dev/null \
  | grep -a -E 'RoActivateInstance|RoOriginateLanguageException|Windows\.Security|Windows\.Foundation|80040154' \
  > "$OUT" &
SECS=${1:-120}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*10))s avail=${avail}MB outkb=$(du -k "$OUT" 2>/dev/null | cut -f1)"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
sleep 1
echo "=== distinct CLSIDs requested (with failures) ==="
grep -a -o -E '\{[0-9A-Fa-f-]{36}\}' "$OUT" | sort | uniq -c | sort -rn | head -25
echo COMCATCH_DONE
