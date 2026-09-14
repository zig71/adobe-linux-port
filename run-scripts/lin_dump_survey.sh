#!/bin/bash
# Dump exception + module list for every crash report.
set -u
D=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Crashpad/reports
ls -la "$D" | head -12
for f in "$D"/*.dmp; do
  echo "== $f"
  python3 /tmp/parse_dmp.py "$f" 2>&1 | grep -a -E '^==|thread=|msedge|d3d11|dxgi|dcomp|oneauth|angle|swiftshader|DWrite|dxcompiler' | head -30
done
echo BTALL_DONE
