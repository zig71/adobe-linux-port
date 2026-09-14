#!/bin/bash
# Full parse of the big DWrite crash dump.
set -u
python3 /tmp/parse_dmp.py '/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Adobe/webview2/Creative_Cloud_Set-Up.exe/EBWebView/Crashpad/reports/414305f8-b065-4fc3-8ffc-de58536e9e9e.dmp' 2>&1 | head -60
echo BIGDONE
