#!/bin/bash
# App launch with DXVK debug logging; extract shared-resource failures.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_LOG_LEVEL=debug
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-dbg.out 2>&1 < /dev/null &
sleep 90
echo "=== dxvk errors ==="
grep -a -E 'err:' /tmp/cc-dbg.out 2>/dev/null | grep -av -E 'EDID|colorimetry|OpenVR' | head -12
echo "=== shared/keyed/nt lines ==="
grep -a -iE 'shared|keyed|NTHANDLE|export|external' /tmp/cc-dbg.out 2>/dev/null | grep -a -iE 'err|warn|fail|unable|cannot|invalid|unsupported' | head -10
echo DBG_DONE
