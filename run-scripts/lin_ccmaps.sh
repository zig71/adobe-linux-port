#!/bin/bash
# Fresh DXVK-state launch + GPU module census via map_files.
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
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-maps.out 2>&1 < /dev/null &
sleep 45
G=$(pgrep -f 'type=gpu-process' 2>/dev/null | head -1)
echo "gpu=$G"
if [ -n "$G" ]; then
  ls -l /proc/$G/map_files/ 2>/dev/null | grep -a -iE 'd3d11|dxgi|d3d9|vulkan-1|swiftshader|libEGL|libGLES' | grep -aoE '(d3d11|dxgi|d3d9|vulkan-1|swiftshader|libEGL|libGLESv2)[^ ]*|drive_c[^ ]*|install[^ ]*' | sort -u | head -14
fi
echo MAPS_DONE
