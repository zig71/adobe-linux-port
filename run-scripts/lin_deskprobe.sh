#!/bin/bash
# Run the dxgi probe on the desktop session.
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export WINEDEBUG=-all
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json
echo "DISPLAY_IS=$DISPLAY"
echo "AUTH_IS=${XAUTHORITY:-unset}"
lab_x_sanity
timeout 60 /home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine \
  /home/kubuntu/adobe-wine-lab/tests/build/probes/probe_dxgi.exe 2>/dev/null \
  | grep -a -E 'D3D11_HARDWARE|D3D11_WARP|ADAPTER_COUNT|PROBE_RESULT' | head -6
echo DESKPROBE_DONE
