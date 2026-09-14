#!/bin/bash
# Hunt stale OS-identity strings in Adobe-owned state (poisoned cache?).
set -u
P=/home/kubuntu/adobe-wine-lab/prefix-wv2
echo "=== files mentioning 19045/Professional under Adobe paths ==="
timeout 60 grep -a -r -l -E '19045|Windows 10 Pro' \
  "$P/drive_c/users/kubuntu/AppData/Local/Adobe" \
  "$P/drive_c/users/kubuntu/AppData/Roaming/Adobe" \
  "$P/drive_c/ProgramData/Adobe" 2>/dev/null | head -10
echo "(end files)"
echo "=== Adobe registry keys with OS strings ==="
export WINEPREFIX=$P
export DISPLAY=:99
unset XAUTHORITY
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
timeout 60 "$W" reg query 'HKCU\Software\Adobe' /s 2>/dev/null | grep -a -i -E 'windows|build|version|os' | head -15
echo "(end reg)"
echo HUNT2_DONE
