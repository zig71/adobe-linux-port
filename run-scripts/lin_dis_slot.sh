#!/bin/bash
# Find the import entry owning IAT slot 0x1937110a0.
set -u
grep -a -n -i -E '1937110a0|337110a0' /tmp/msedge_imports.txt | head -5
echo "=== nearby DLL ==="
n=$(grep -a -n -i '1937110a0' /tmp/msedge_imports.txt | head -1 | cut -d: -f1)
if [ -n "$n" ]; then
  sed -n "$((n-60)),$((n+3))p" /tmp/msedge_imports.txt | grep -a -E 'DLL Name' | tail -2
  sed -n "$((n-3)),$((n+1))p" /tmp/msedge_imports.txt
fi
echo SLOTDONE
