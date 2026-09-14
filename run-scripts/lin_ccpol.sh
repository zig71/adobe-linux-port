#!/bin/bash
# Software-policy test: Chromium GPU disabled by policy, fresh launch, watch.
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
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-pol.out 2>&1 < /dev/null &
sleep 25
timeout 40 "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' >> /tmp/cc-pol.out 2>&1
sleep 75
echo "=== CEF tail ==="
tail -6 ~/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/CEF.log 2>/dev/null | grep -aE 'crashed|FATAL|backing|mailbox|software|Software|swiftshader|SwiftShader|disabled|Disabled' | cut -c1-130 | head -6
echo "=== spectacle ==="
spectacle -b -n -o /tmp/cc-pol.png 2>/dev/null; ls -la /tmp/cc-pol.png 2>/dev/null
echo POL_DONE
