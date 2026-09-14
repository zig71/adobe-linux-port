#!/bin/bash
# Explicit virtual desktop + app launch. Desktop first, app second.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
pkill -9 -f 'Creative Clou[d]' 2>/dev/null; sleep 1
setsid "$W" explorer /desktop=Default,1280x960 > /tmp/cc-vdtop.out 2>&1 < /dev/null &
sleep 8
echo "=== desktop window ==="
xwininfo -root -children 2>/dev/null | grep -a -iE 'desktop|explorer' | head -3
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-vdapp.out 2>&1 < /dev/null &
sleep 25
timeout 40 "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' >> /tmp/cc-vdapp.out 2>&1
sleep 60
echo "=== spectacle ==="
spectacle -b -n -o /tmp/cc-vdtop.png 2>/dev/null; ls -la /tmp/cc-vdtop.png 2>/dev/null
echo VDTOP_DONE
