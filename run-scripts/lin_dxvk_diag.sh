#!/bin/bash
# DXVK diagnostics run: capture shared-resource/external-memory errors.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json
export DXVK_LOG_LEVEL=info
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f /tmp/cc-dxvk.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-dxvk.out 2>&1 < /dev/null &
SECS=${1:-150}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  echo "t=$((i*10))s avail=${avail}MB outkb=$(du -k /tmp/cc-dxvk.out 2>/dev/null | cut -f1)"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== shared/external memory diagnostics ==="
grep -a -i -E 'err|shared|external|export|import memory|NT handle|opaque fd' /tmp/cc-dxvk.out 2>/dev/null | grep -a -v -E 'OpenVR|dzn is not' | cut -c1-160 | sort | uniq -c | sort -rn | head -25
echo DXVKDIAG_DONE
