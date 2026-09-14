#!/bin/bash
# Identify the import behind IAT slot 0x1937110a0.
set -u
F=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll
x86_64-w64-mingw32-objdump -p "$F" 2>/dev/null > /tmp/msedge_imports.txt
wc -l < /tmp/msedge_imports.txt
grep -a -n -B4 -A40 'DLL Name: KERNEL32.dll' /tmp/msedge_imports.txt | grep -a -E 'DLL Name|37110|vma|Hint' | head -20
echo IMPDONE
