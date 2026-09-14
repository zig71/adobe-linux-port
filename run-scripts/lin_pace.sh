#!/bin/bash
# Sample WAL size + HTTP count every 60s x10 to characterize sync pacing.
set -u
P=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/LocalLow/Adobe/GrowthSDK/Production/x64/content/000.071/assets.db-wal
L=/home/kubuntu/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/ACC.log
for i in $(seq 1 10); do
  sz=$(stat -c %s "$P" 2>/dev/null || echo -1)
  http=$(tr -d '\000' < "$L" 2>/dev/null | grep -a -c 'HTTPConnector')
  echo "t=$((i*60))s wal=$sz http_lines=$http"
  sleep 60
done
echo PACE_DONE
