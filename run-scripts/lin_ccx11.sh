#!/bin/bash
# Short x11drv trace of app startup; mine teardown sequence for 0x400001.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all,+win,+x11drv
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-x11.out 2>&1 < /dev/null &
sleep 25
echo "=== size ==="
ls -la /tmp/cc-x11.out 2>/dev/null
echo "=== 400001 mentions ==="
grep -a -c '400001' /tmp/cc-x11.out 2>/dev/null
grep -a '400001' /tmp/cc-x11.out 2>/dev/null | grep -aoE '(create|destroy|unmap|map|visual)[A-Za-z_ ]{0,40}' | sort | uniq -c | head -10
echo X11MINE_DONE
