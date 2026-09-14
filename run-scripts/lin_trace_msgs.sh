#!/bin/bash
# Trace the window messages the Creative Cloud installer's windows receive,
# to find why ViewMediatorUIWin's onWindowResize never fires under Wine.
#
# Runs the installer for a bounded time with +msg/+win and reduces the trace to
# the message classes that matter for a top-level window being shown and sized.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-fresh}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
INSTALLER=/home/kubuntu/Downloads/Creative_Cloud_Set-Up.exe
SECS=${1:-35}
OUT=/tmp/msg-trace.out

# Kill a previously started installer without matching this script's own argv.
pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$OUT"

cd /home/kubuntu/Downloads
WINEDEBUG=+msg,+win timeout "$SECS" "$W" "$INSTALLER" > "$OUT" 2>&1
echo "trace_lines=$(wc -l < "$OUT")"

echo "=== window creation ==="
grep -E 'win:.*(CreateWindow|create_window|WIN_CreateWindow)' "$OUT" | head -20

echo "=== message classes (counts) ==="
grep -oE 'msg:MSG_[A-Z_]+' "$OUT" | sort | uniq -c | sort -rn | head -30

echo "=== the messages that drive layout/visibility ==="
grep -E 'MSG_(SIZE|WINDOWPOSCHANGED|SHOWWINDOW|CREATE|PAINT|MOVE|NCCALCSIZE|GETMINMAXINFO|ACTIVATE|SETFOCUS)\b' "$OUT" | head -40

echo "=== dispatch targets (window handles seen) ==="
grep -oE 'msg:DispatchMessage|msg:PeekMessage' "$OUT" | sort | uniq -c | head
grep -oE 'hwnd=[0-9A-Fa-f]+' "$OUT" | sort | uniq -c | sort -rn | head -15

echo MSG_TRACE_DONE
