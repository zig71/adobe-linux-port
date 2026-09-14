#!/bin/bash
# CC app launch inside a Wine virtual desktop on :0.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-vd.out 2>&1 < /dev/null &
sleep 75
echo "=== xerrors ==="
grep -a -c 'X Error' /tmp/cc-vd.out 2>/dev/null
echo "=== top-level windows ==="
xwininfo -root -children 2>/dev/null | grep -aE '0x[0-9a-f]+' | grep -av -E '1x1|3x3|8x8|10x10' | head -6
echo VDONE
