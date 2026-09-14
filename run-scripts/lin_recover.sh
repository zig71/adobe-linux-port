#!/bin/bash
# Post-reboot lab recovery check + repair.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

echo "=== 64-bit identity ==="
"$W" reg query 'HKLM\Software\Microsoft\Windows NT\CurrentVersion' 2>/dev/null | grep -E 'DisplayVersion|CurrentBuildNumber|ProductName|EditionId'
echo "=== 32-bit identity ==="
"$W" reg query 'HKLM\Software\WOW6432Node\Microsoft\Windows NT\CurrentVersion' 2>/dev/null | grep -E 'DisplayVersion|CurrentBuildNumber|ProductName|EditionId'
echo "=== webview2 keys ==="
"$W" reg query 'HKLM\Software\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}' /v pv 2>&1 | head -2
"$W" reg query 'HKLM\Software\WOW6432Node\Microsoft\EdgeUpdate\ClientState\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}' /v EBWebView 2>&1 | head -2
echo "=== dxvk files ==="
P=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/windows
file "$P/system32/d3d11.dll" 2>/dev/null | grep -o -E 'x86-64|i386'
file "$P/syswow64/d3d11.dll" 2>/dev/null | grep -o -E 'x86-64|i386|WINE'
echo "=== fonts ==="
ls "$P/../..//drive_c/windows/Fonts/" 2>/dev/null | wc -l
ls /home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/windows/Fonts/ 2>/dev/null | wc -l
echo "=== xvfb ==="
pgrep -f 'Xvfb :99' >/dev/null || setsid Xvfb :99 -screen 0 1280x960x24 -nolisten tcp >/tmp/xvfb.log 2>&1 < /dev/null
sleep 3
DISPLAY=:99 xdpyinfo 2>&1 | grep dimensions || echo XVFB_DEAD
echo "=== swap ==="
swapon --show | head -3
free -m | head -2
echo RECOVER_CHECK_DONE
