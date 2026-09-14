#!/bin/bash
# Definitive instance census: every installer/browser/wineserver process.
set -u
echo "=== installers ==="
pgrep -af 'Creative_Cloud' | cut -c1-120 | head -6
echo "=== browsers ==="
pgrep -c -f 'msedgewebview2\.exe' || echo 0
echo "=== wineservers ==="
pgrep -af 'wineserver' | cut -c1-100 | head -4
echo "=== recent lock lines ==="
tr -d '\000' < /home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log 2>/dev/null | grep -a -c 'Failed to acquire'
echo CENSUS2_DONE
