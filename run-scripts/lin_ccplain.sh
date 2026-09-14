#!/bin/bash
# Plain direct launch (no virtual desktop), capture result.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-plain.out 2>&1 < /dev/null &
sleep 120
spectacle -b -n -o /tmp/cc-plain.png 2>/dev/null; ls -la /tmp/cc-plain.png 2>/dev/null
echo PLAIN_DONE
