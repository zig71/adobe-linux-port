#!/bin/bash
# Wide view ending at the repeated caller frame.
set -u
F=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll
x86_64-w64-mingw32-objdump -d --start-address=0x181082700 --stop-address=0x181082820 "$F" 2>/dev/null | head -80
echo WIDEDONE
