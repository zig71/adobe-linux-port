#!/bin/bash
# Apply the no-d3d12 fallback config and verify.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
"$W" regedit /s /tmp/no_d3d12.reg
echo "import_rc=$?"
"$W" reg query 'HKCU\Software\Wine\DllOverrides' 2>/dev/null | grep -E 'd3d1|dxgi'
echo FALLBACKCFG_DONE
