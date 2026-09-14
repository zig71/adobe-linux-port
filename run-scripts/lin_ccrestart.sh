#!/bin/bash
# Clean-restart recipe (the one that painted): kill all, explorer, app, nudge.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
pkill -9 -f 'Creative Clou[d]' 2>/dev/null
pkill -9 -f 'Adobe Desktop Servic[e]' 2>/dev/null
pkill -9 -f 'AdobeIPCBroke[r]' 2>/dev/null
pkill -9 -f 'CoreSyn[c]' 2>/dev/null
pkill -9 -f 'CCXProces[s]' 2>/dev/null
sleep 2
setsid "$W" explorer /desktop=Default,1280x960 > /tmp/cc-rs-exp.out 2>&1 < /dev/null &
sleep 8
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-rs-app.out 2>&1 < /dev/null &
sleep 30
timeout 40 "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' >> /tmp/cc-rs-app.out 2>&1
sleep 150
spectacle -b -n -o /tmp/cc-restart.png 2>/dev/null; ls -la /tmp/cc-restart.png 2>/dev/null
echo RESTART_DONE
