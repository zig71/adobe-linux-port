#!/bin/bash
# Desktop probe with error channels visible.
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export WINEDEBUG=+seh,+pid
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json
timeout 60 /home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine \
  /home/kubuntu/adobe-wine-lab/tests/build/probes/probe_dxgi.exe 2>&1 | head -25
echo "RC=${PIPESTATUS[0]}"
echo DESKDBG_DONE
