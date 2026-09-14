#!/bin/bash
# Trace window messages for the Creative Cloud installer's own windows.
#
# Wine's message spy lives on the "message" debug channel (dlls/win32u/spy.c:
# spy_init() returns FALSE unless TRACE_ON(message)); +msg traces something else
# entirely and produces nothing useful here.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-fresh}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
INSTALLER=/home/kubuntu/Downloads/Creative_Cloud_Set-Up.exe
SECS=${1:-160}
RAW=/tmp/msg-raw.out

pkill -9 -f 'Set[-]Up\.exe' 2>/dev/null
sleep 2
rm -f "$RAW"

cd /home/kubuntu/Downloads
WINEDEBUG=+message timeout "$SECS" "$W" "$INSTALLER" > "$RAW" 2>&1
echo "raw_lines=$(wc -l < "$RAW")"

echo "=== the installer's own windows ==="
grep -aE 'AdobeInstallerWindowClass|AdobeWebBrowserWindowClass|Creative Cloud Installer' "$RAW" | head -20
echo "=== window handles for those classes ==="
grep -aoE '\(0x[0-9a-f]+\) L"Adobe[A-Za-z]*"' "$RAW" | sort -u | head -10

echo "=== ALL WM_SHOWWINDOW ==="
grep -a 'WM_SHOWWINDOW' "$RAW" | head -20
echo "=== ALL WM_WINDOWPOSCHANGED ==="
grep -a 'WM_WINDOWPOSCHANGED' "$RAW" | head -20
echo "=== WM_SIZE (first 30) ==="
grep -a 'WM_SIZE' "$RAW" | head -30
echo "=== tree of windows created with sizes ==="
grep -aE 'create_window|WIN_CreateWindowEx' "$RAW" | head -5
echo MSGTRACE_DONE
