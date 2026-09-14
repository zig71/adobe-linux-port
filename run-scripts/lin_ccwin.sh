#!/bin/bash
# Traced CC app launch (+win), 45s, then summarize window lifecycle.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all,+win
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-wintrace.out 2>&1 < /dev/null &
sleep 45
echo "=== xerrors ==="
grep -a -c 'X Error' /tmp/cc-wintrace.out 2>/dev/null
grep -a -A2 'X Error' /tmp/cc-wintrace.out 2>/dev/null | grep -aE 'opcode|Resource' | head -4
echo "=== win ops ==="
grep -a -oiE 'createwindow[A-Za-z]*|destroywindow|unmapwindow|showwindow[^ ]*|setwindowpos' /tmp/cc-wintrace.out 2>/dev/null | sort | uniq -c | head -10
echo WINTRACE_DONE
