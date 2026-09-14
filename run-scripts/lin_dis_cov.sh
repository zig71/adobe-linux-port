#!/bin/bash
# Probe which VMAs objdump will disassemble.
set -u
F=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll
for a in 0x188000000 0x188600000 0x18864000 0x18864100 0x1889d9000 0x189000000 0x18a000000 0x18a6fb00; do
  b=$(printf '0x%x' $((a + 16)))
  n=$(x86_64-w64-mingw32-objdump -d --start-address=$a --stop-address=$b "$F" 2>/dev/null | grep -c ':')
  echo "$a -> $n lines"
done
echo PROBEADD_DONE
