#!/bin/bash
# Run the Creative Cloud installer with a working X environment and report how
# far it gets. Sourced from lab_env.sh so the X auth cookie is discovered
# rather than hardcoded.
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
lab_x_sanity || exit 1

OUT=/tmp/cc-run.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
SECS=${1:-210}

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$OUT" "$LOG"

cd /home/kubuntu/Downloads
setsid "$WINE" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &

for i in $(seq 1 $((SECS / 35))); do
  sleep 35
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview2' 2>/dev/null || echo 0)
  rs=$(grep -c 'onWindowResize' "$LOG" 2>/dev/null || echo 0)
  st=$(grep -oE 'state: [A-Z_]+' "$LOG" 2>/dev/null | tail -1)
  echo "t=$((i*35))s installer_log=$n webview2_procs=$wv onWindowResize=$rs last_state=${st:-none}"
done

echo
echo "=== workflow states reached (oracle reaches 7) ==="
grep -oE 'state: [A-Z_]+' "$LOG" 2>/dev/null | sort -u
echo "(end)"
echo
echo "=== onWindowResize (oracle logs this ~20ms after init) ==="
grep -a 'onWindowResize' "$LOG" 2>/dev/null | head -3 || echo "(absent)"
echo
echo "=== installer capture, non-fixme ==="
grep -avE '^[0-9a-f]+:fixme|WARNING: dzn|libEGL|^$' "$OUT" | tail -20
echo RUN_DONE
