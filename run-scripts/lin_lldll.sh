#!/bin/bash
# Module-load trace: which graphics DLLs do installer/browser processes load?
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all,+loaddll
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-lldll.out 2>&1 < /dev/null &
sleep 60
echo "=== graphics module loads ==="
grep -a -iE 'd3d11|dxgi|d3d12|vulkan|libEGL|libGLES|dcomp|opengl|xinput' /tmp/cc-lldll.out 2>/dev/null | grep -aoE '[A-Za-z0-9_.-]+\.dll' | sort -f | uniq -c | sort -rn | head -16
echo LLDLL_DONE
