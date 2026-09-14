#!/bin/bash
# Long, filtered message trace of the Creative Cloud installer under Wine.
#
# The installer takes ~30s to get through its HTTP ingest before it creates its
# window, so a short run sees nothing. +msg is far too voluminous to keep whole,
# so it is filtered to the message classes that drive window sizing and
# visibility -- exactly the ones ViewMediatorUIWin's onWindowResize depends on.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-fresh}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
INSTALLER=/home/kubuntu/Downloads/Creative_Cloud_Set-Up.exe
SECS=${1:-150}
KEEP=/tmp/msg-filtered.out
WINOUT=/tmp/win-filtered.out

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$KEEP" "$WINOUT"

cd /home/kubuntu/Downloads

# Keep only the interesting message classes, plus any window with a real title.
WINEDEBUG=+msg,+win timeout "$SECS" "$W" "$INSTALLER" 2>&1 \
  | grep -aE 'MSG_(SIZE|WINDOWPOSCHANGED|SHOWWINDOW|CREATE|MOVE|NCCALCSIZE|GETMINMAXINFO|ACTIVATE|SETFOCUS|PAINT)|win:WIN_CreateWindowEx.*->L"[^#]' \
  > "$KEEP"

echo "filtered_lines=$(wc -l < "$KEEP")"
echo "=== application windows created (non-system classes) ==="
grep -aE 'win:WIN_CreateWindowEx' "$KEEP" | grep -avE 'L"#[0-9]|WineAppBar|__wine|Shell_TrayWnd|Message"|Default IME|MSCTF' | head -30
echo "=== message classes ==="
grep -aoE 'MSG_[A-Z_]+' "$KEEP" | sort | uniq -c | sort -rn | head -25
echo "=== first 40 kept lines ==="
head -40 "$KEEP"
echo MSG_FILTER_DONE
