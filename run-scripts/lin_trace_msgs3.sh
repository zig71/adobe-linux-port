#!/bin/bash
# Filtered +msg trace of the Creative Cloud installer.
#
# Wine's message spy (dlls/win32u/spy.c:2629) prints
#     "%*s(%p) %-16s [%04x] %s dispatched  wp=%08lx lp=%08lx"
# so the message name appears as WM_SIZE, not MSG_SIZE, on the msg channel.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-fresh}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
INSTALLER=/home/kubuntu/Downloads/Creative_Cloud_Set-Up.exe
SECS=${1:-150}
KEEP=/tmp/msg2.out

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$KEEP"

cd /home/kubuntu/Downloads
WINEDEBUG=+msg timeout "$SECS" "$W" "$INSTALLER" 2>&1 \
  | grep -aE 'WM_(SIZE|WINDOWPOSCHANGED|WINDOWPOSCHANGING|SHOWWINDOW|CREATE|MOVE|PAINT|ACTIVATE|NCCALCSIZE|GETMINMAXINFO)\b' \
  > "$KEEP"

echo "kept=$(wc -l < "$KEEP")"
echo "=== messages for the installer's own windows ==="
grep -aE 'AdobeInstallerWindowClass|AdobeWebBrowserWindowClass' "$KEEP" | head -40
echo "=== all WM_SIZE ==="
grep -a 'WM_SIZE' "$KEEP" | head -20
echo "=== all WM_CREATE ==="
grep -a 'WM_CREATE' "$KEEP" | head -10
echo "=== all WM_SHOWWINDOW ==="
grep -a 'WM_SHOWWINDOW' "$KEEP" | head -10
echo "=== class of window names seen ==="
grep -aoE '"[A-Za-z0-9_ ]+"' "$KEEP" | sort | uniq -c | sort -rn | head -20
echo MSG2_DONE
