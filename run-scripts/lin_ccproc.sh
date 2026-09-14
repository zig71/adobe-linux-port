#!/bin/bash
# Bootstrapper re-run with process tracing to capture the LAUNCH_CCD cmdline.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all,+process
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-proc.out 2>&1 < /dev/null &
sleep 240
echo "=== child launches ==="
grep -a -iE 'starting process|createprocess|command line' /tmp/cc-proc.out 2>/dev/null | grep -a -iE 'creative|adobe|setup|hdbox' | cut -c1-220 | head -10
echo PROC_DONE
