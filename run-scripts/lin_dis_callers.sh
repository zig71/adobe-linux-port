#!/bin/bash
# Disassemble the fatal-path callers.
set -u
F=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll
for a in 0x1810826e0 0x1810827f0 0x1810ae7c0; do
  b=$(printf '0x%x' $((a + 96)))
  echo "=== $a ==="
  x86_64-w64-mingw32-objdump -d --start-address=$a --stop-address=$b "$F" 2>/dev/null | tail -12
done
echo CALLERSDONE
