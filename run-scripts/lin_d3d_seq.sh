#!/bin/bash
# D3D-sequence run: narrow channels to see the GPU child's last D3D calls.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=+d3d11,+dxgi,+d3d
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f /tmp/cc-d3d.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-d3d.out 2>&1 < /dev/null &
SECS=${1:-150}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  echo "t=$((i*10))s wv2=$wv avail=${avail}MB outkb=$(du -k /tmp/cc-d3d.out 2>/dev/null | cut -f1)"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1
echo "=== last D3D lines before deaths ==="
ls -la /tmp/cc-d3d.out
echo D3DSEQ_DONE
