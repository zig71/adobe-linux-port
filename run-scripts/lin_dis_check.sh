#!/bin/bash
# Disassemble around the int3 and list rip-relative string references.
set -u
F=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll
x86_64-w64-mingw32-objdump -d --start-address=0x1889d8e00 --stop-address=0x1889d900c "$F" 2>/dev/null > /tmp/check_dis.txt
wc -l < /tmp/check_dis.txt
grep -a -E 'lea.*rip' /tmp/check_dis.txt | head -20
echo DIS_DONE
