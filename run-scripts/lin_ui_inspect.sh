#!/bin/bash
# Inspect the newest UI bundle entry page.
set -u
F=$(find /home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Temp -maxdepth 2 -name 'index.html' 2>/dev/null | head -1)
echo "file=$F"
wc -c "$F"
grep -a -o -E '<script[^>]*src="[^"]+"|<link[^>]*href="[^"]+"' "$F" 2>/dev/null | head -10
echo UIINSPECT_DONE
