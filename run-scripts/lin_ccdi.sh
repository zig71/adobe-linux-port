#!/bin/bash
# CCDI-mode launch (oracle parity attempt without install-specific GUIDs).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
pkill -9 -f 'Creative Clou[d]' 2>/dev/null; sleep 2
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' --mode=CCDI > /tmp/cc-ccdi.out 2>&1 < /dev/null &
sleep 150
spectacle -b -n -o /tmp/cc-ccdi.png 2>/dev/null; ls -la /tmp/cc-ccdi.png 2>/dev/null
echo CCDI_DONE
