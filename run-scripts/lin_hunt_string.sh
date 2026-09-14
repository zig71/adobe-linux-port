#!/bin/bash
# Hunt the 'Current OS is not supported' string: which on-disk module logs it?
# Searches the prefix, Downloads, and unpacked setup. Read-only, low memory.
set -u
echo "=== unpacked bootstrapper ==="
grep -a -l 'Current OS is not supported' /home/kubuntu/adobe-wine-lab/work-unpack/setup.exe 2>/dev/null || echo "(not in bootstrapper)"
echo
echo "=== prefix (drive_c) ==="
timeout 100 grep -a -r -l 'Current OS is not supported' /home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/ 2>/dev/null | head -10
echo "(end prefix)"
echo
echo "=== Downloads ==="
timeout 60 grep -a -r -l 'Current OS is not supported' /home/kubuntu/Downloads/ 2>/dev/null | head -5
echo "(end downloads)"
echo
echo "=== Adobe dirs anywhere in home ==="
timeout 60 find /home/kubuntu -ipath '*adobe*' -name '*.log' 2>/dev/null | head -10
echo HUNT_DONE
