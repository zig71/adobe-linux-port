#!/bin/bash
# D3D12-absent test in the isolated staging prefix: does hiding Microsoft's
# d3d12core stop the GPU int3 loop? Fully reversible (rename back).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-staging
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
P=/home/kubuntu/adobe-wine-lab/prefix-staging/drive_c/windows
for f in system32/d3d12.dll system32/d3d12core.dll syswow64/d3d12.dll syswow64/d3d12core.dll; do
  [ -f "$P/$f" ] && mv "$P/$f" "$P/$f.nope" && echo "hid $f"
done
"$W" reg delete 'HKCU\Software\Wine\AppDefaults\msedgewebview2.exe' /v Version /f 2>&1 | tail -1
R="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Crashpad/reports"
rm -f "$R"/*.dmp 2>/dev/null

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-nod3d12.out 2>&1 < /dev/null &
for i in $(seq 1 18); do
  sleep 5
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  dmps=$(ls "$R"/*.dmp 2>/dev/null | wc -l)
  echo "t=$((i*5))s wv2=$wv dumps=$dmps"
done
for f in system32/d3d12.dll system32/d3d12core.dll syswow64/d3d12.dll syswow64/d3d12core.dll; do
  [ -f "$P/$f.nope" ] && mv "$P/$f.nope" "$P/$f" && echo "restored $f"
done
echo NOD3D12_DONE
