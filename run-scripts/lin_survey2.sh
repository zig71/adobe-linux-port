#!/bin/bash
# Parse all current crash reports: codes + modules.
set -u
D=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Crashpad/reports
for f in "$D"/*.dmp; do
  sz=$(stat -c %s "$f" 2>/dev/null || echo 0)
  if [ "$sz" -gt "1000" ]; then
    python3 /tmp/parse_dmp.py "$f" 2>&1 | grep -a -E '^==|thread=|d3d11|dxgi|dcomp|oneauth|DWrite|dxcompiler|angle|swiftshader|libEGL|libGLES|vulkan|wined3d' | head -25
  fi
done
echo SURVEY2_DONE
