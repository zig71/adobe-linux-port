#!/bin/bash
# Did the installer ask Wine to show its window?
#
# The message trace shows the installer's top-level window is created, sized,
# activated and focused, but never receives WM_SHOWWINDOW, while its browser
# children do. This traces the API calls that would produce that message --
# ShowWindow / SetWindowPos / ShowWindowAsync -- to establish whether the
# application asked and Wine dropped it, or the application never asked.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-fresh}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
INSTALLER=/home/kubuntu/Downloads/Creative_Cloud_Set-Up.exe
SECS=${1:-90}
OUT=/tmp/winapi.out

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$OUT"

cd /home/kubuntu/Downloads
# Relay is enormously verbose; filter as it streams so only the interesting
# entry points survive.
WINEDEBUG=+relay timeout "$SECS" "$W" "$INSTALLER" 2>&1 \
  | grep -aE 'ShowWindow|SetWindowPos|ShowWindowAsync|ShowOwnedPopups|CreateWindowEx.*(Adobe|#32769)' \
  > "$OUT"

echo "lines=$(wc -l < "$OUT")"
echo "=== calls, deduplicated by shape ==="
sed -E 's/^[0-9a-f]+://; s/ret=[0-9a-f]+//; s/\(0x[0-9a-f]+\)/(HWND)/g; s/[0-9a-f]{8,}/PTR/g' "$OUT" \
  | sort | uniq -c | sort -rn | head -30
echo "=== raw sample (first 25) ==="
head -25 "$OUT"
echo WINAPI_DONE
