#!/bin/bash
# Post-reboot recovery check: confirm OOM cause, clean strays, verify lab state.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

echo "=== stray processes ==="
pgrep -af 'msedgewebview2|Set-Up' | head -8 || echo "(none)"
pkill -9 -f 'msedgewebview2' 2>/dev/null || true
pkill -9 -f 'Set-Up' 2>/dev/null || true
sleep 1
echo "after_kill_wv2=$(pgrep -c -f msedgewebview2 || echo 0)"

echo
echo "=== OOM evidence from previous boot ==="
journalctl -b -1 --no-pager 2>/dev/null | grep -iE 'oom|killed process|out of memory' | tail -8 || echo "(no previous-boot journal)"

echo
echo "=== memory now ==="
free -m | head -2

echo
echo "=== prefix OS identity survived? ==="
"$W" reg query 'HKLM\Software\Microsoft\Windows NT\CurrentVersion' 2>/dev/null | grep -E 'CurrentBuild|ProductName|UBR|CurrentVersion'

echo
echo "=== WebView2 staging intact? ==="
ls ~/adobe-wine-lab/webview2/ | head -3
"$W" reg query 'HKLM\Software\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}' 2>/dev/null | head -6

echo
echo "=== WAM.log from fatal run ==="
L="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
wc -l < "$L" 2>/dev/null || echo "(no log)"
tr -d '\000' < "$L" 2>/dev/null | grep -a 'not supported' | cut -c1-120 | head -2
tr -d '\000' < "$L" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "=== screenshots kept? ==="
ls -la ~/adobe-wine-lab/runs/cc-headless/ | head -8
echo RECOVERY_DONE
