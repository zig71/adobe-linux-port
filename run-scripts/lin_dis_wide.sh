#!/bin/bash
# Disassemble a wide window around the int3 and list all call targets,
# so the crashing function's imports name its subsystem.
set -u
F=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll
x86_64-w64-mingw32-objdump -d --start-address=0x1889d7c00 --stop-address=0x1889d9100 "$F" 2>/dev/null > /tmp/check_wide.txt
wc -l < /tmp/check_wide.txt
grep -a -o -E 'call +0x[0-9a-f]+' /tmp/check_wide.txt | sort | uniq -c | sort -rn | head -30
echo WIDEDONE
