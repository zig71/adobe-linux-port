#!/bin/bash
# Read the fatal run's installer log tail.
set -u
L=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log
echo "lines=$(wc -l < "$L" 2>/dev/null || echo none)"
echo "--- not-supported? ---"
tr -d '\000' < "$L" 2>/dev/null | grep -a 'not supported' | cut -c1-160 | head -3
echo "(end)"
echo "--- workflow states ---"
tr -d '\000' < "$L" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "(end)"
echo "--- onWindowResize ---"
tr -d '\000' < "$L" 2>/dev/null | grep -ac 'onWindowResize'
echo "--- last 8 semantic lines ---"
tr -d '\000' < "$L" 2>/dev/null | grep -aE '\[(INFO|WARN|ERROR)\]' | tail -8 | cut -c1-170
echo "--- screenshots ---"
ls -la ~/adobe-wine-lab/runs/cc-headless/
echo WAMTAIL_DONE
