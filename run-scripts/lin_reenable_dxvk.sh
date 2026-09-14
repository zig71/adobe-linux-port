#!/bin/bash
# Re-enable DXVK native overrides.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
"$W" reg add 'HKCU\Software\Wine\DllOverrides' /v d3d11 /d native /f 2>&1 | tail -1
"$W" reg add 'HKCU\Software\Wine\DllOverrides' /v dxgi /d native /f 2>&1 | tail -1
"$W" reg query 'HKCU\Software\Wine\DllOverrides' 2>/dev/null | grep -E 'd3d11|dxgi'
echo REENABLE_DONE
