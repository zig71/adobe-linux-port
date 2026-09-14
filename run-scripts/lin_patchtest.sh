#!/bin/bash
# Patched-driver efficacy: 2 fresh app launches, count X errors + windows.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu
for round in 1 2; do
  setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-pat$round.out 2>&1 < /dev/null &
  sleep 55
  echo "round $round xerrors=$(grep -a -c 'X Error' /tmp/cc-pat$round.out 2>/dev/null)"
  pkill -9 -f 'Creative Clou[d]' 2>/dev/null; sleep 3
done
echo PATCH_TEST_DONE
