#!/bin/bash
# Search runtime DLLs for the wide "Servic..." exception text.
set -u
R=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32
strings -el "$R/oneauth.dll" 2>/dev/null | grep -a -i -E '^Servic.{0,80}' | head -10
echo "=== msedge Servic* near auth ==="
strings -el "$R/msedge.dll" 2>/dev/null | grep -a -i -E 'Service.{0,60}(fail|error|unavail|not |missing)' | head -10
echo STRDONE
