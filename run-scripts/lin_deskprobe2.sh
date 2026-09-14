#!/bin/bash
# Desktop probe with markers.
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export WINEDEBUG=-all
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json
echo "MARKER_START disp=$DISPLAY"
timeout 60 /home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine \
  /home/kubuntu/adobe-wine-lab/tests/build/probes/probe_dxgi.exe
echo "MARKER_RC=$?"
echo MARKER_DONE
