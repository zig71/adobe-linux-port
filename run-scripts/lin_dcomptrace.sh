#!/bin/bash
# DComp call trace: does the installer/browser touch DirectComposition?
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all,+dcomp
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-dcomp.out 2>&1 < /dev/null &
sleep 70
echo "=== dcomp calls ==="
grep -a -c 'dcomp' /tmp/cc-dcomp.out 2>/dev/null
grep -a -oE '(DCompositionCreate[A-Za-z0-9]*|dcomp:[a-z_]*)' /tmp/cc-dcomp.out 2>/dev/null | sort | uniq -c | head -12
echo "=== history check ==="
H=$(ls -td "$WINEPREFIX"/drive_c/users/kubuntu/AppData/Local/Temp/\{*-\}*/EBWebView/Default/History 2>/dev/null | head -1)
echo "hist=$H"
[ -n "$H" ] && cp "$H" /tmp/h2.db 2>/dev/null && sqlite3 /tmp/h2.db "SELECT url FROM urls ORDER BY last_visit_time DESC LIMIT 4;" 2>&1 | cut -c1-90
echo DCOMP_DONE
