#!/bin/bash
# DXVK log trace: does Chromium create/present any swapchain?
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
export DXVK_LOG_LEVEL=info
export DXVK_LOG_PATH=/tmp
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f /tmp/dxvk-*.log
cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-dxvklog.out 2>&1 < /dev/null &
sleep 60
echo "=== dxvk logs ==="
ls -la /tmp/dxvk-*.log 2>/dev/null | head -6
echo "=== swapchain/present/err lines ==="
grep -a -iE 'swapchain|present|failed|err|warn' /tmp/dxvk-*.log 2>/dev/null | grep -aoE '(SwapChain|Present|Failed|failed|ERR|err|WARN|warn)[^,;]*' | sort | uniq -c | sort -rn | head -16
echo DXVKLOG_DONE
