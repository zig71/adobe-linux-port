#!/bin/bash
# Single-launch test: NO nudge. Does core init succeed and UI advance?
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
setsid "$W" explorer /desktop=Default,1280x960 > /tmp/cc-s1-exp.out 2>&1 < /dev/null &
sleep 8
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-s1.out 2>&1 < /dev/null &
sleep 210
echo "=== core init? ==="
tr -d '\000' < ~/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/ACC.log 2>/dev/null | grep -a -E 'nitialize core library' | cut -c1-100 | tail -3
spectacle -b -n -o /tmp/cc-single.png 2>/dev/null; ls -la /tmp/cc-single.png 2>/dev/null
echo SINGLE_DONE
