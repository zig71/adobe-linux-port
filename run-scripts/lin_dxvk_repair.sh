#!/bin/bash
# Repair inverted DXVK install: x64 -> system32, x32 -> syswow64,
# restore Wine originals from .old where DXVK has no file, set overrides.
set -u
P=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/windows
D=/tmp/dxvk-1.10.3
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

echo "=== placing DXVK dlls correctly ==="
cp -f "$D/x64/d3d11.dll" "$P/system32/d3d11.dll"
cp -f "$D/x64/dxgi.dll" "$P/system32/dxgi.dll"
cp -f "$D/x32/d3d11.dll" "$P/syswow64/d3d11.dll"
cp -f "$D/x32/dxgi.dll" "$P/syswow64/dxgi.dll"
file "$P/system32/d3d11.dll" "$P/syswow64/d3d11.dll" | head -4

echo "=== overrides to native ==="
"$W" reg add 'HKCU\Software\Wine\DllOverrides' /v d3d11 /d native /f 2>&1 | tail -1
"$W" reg add 'HKCU\Software\Wine\DllOverrides' /v dxgi /d native /f 2>&1 | tail -1
"$W" reg query 'HKCU\Software\Wine\DllOverrides' 2>/dev/null | grep -E 'd3d11|dxgi'

echo "=== probe check ==="
timeout 60 "$W" ~/adobe-wine-lab/tests/build/probes/probe_dxgi.exe 2>&1 | head -12
echo DXVKREPAIR_DONE
