#!/bin/bash
# Disassemble the new int3 site and the caller return site.
set -u
F=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll
echo "=== int3 site msedge+0xA6FBBD (vma 0x18A6FB90) ==="
x86_64-w64-mingw32-objdump -d --start-address=0x18a6fb40 --stop-address=0x18a6fbc0 "$F" 2>/dev/null | tail -30
echo "=== caller msedge+0x864129 (vma 0x18864100) ==="
x86_64-w64-mingw32-objdump -d --start-address=0x188640e0 --stop-address=0x18864140 "$F" 2>/dev/null | tail -25
echo DIS2DONE
