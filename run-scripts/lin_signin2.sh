#!/bin/bash
# Full reset + clean desktop launch: kill everything, reset wineserver
# (clears stale mutexes), then launch once.
set -u
. /home/kubuntu/adobe-wine-lab/scripts/linux/lab_env.sh
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export WINEDEBUG=-all
export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json
unset WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

lab_x_sanity
pkill -9 -f 'Set-Up' 2>/dev/null
pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 3
"$W" wineserver -k 2>/dev/null
sleep 4
echo "cleaned. running: $(pgrep -c -f 'Set-Up' 2>/dev/null || echo 0) installers"
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$LOG" /tmp/cc-signin.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up_747d.exe > /tmp/cc-signin.out 2>&1 < /dev/null &
echo "LAUNCHED display=$DISPLAY"
for i in $(seq 1 30); do
  sleep 10
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  st=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | tail -1)
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  al=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -ac 'showErrorAlert')
  echo "t=$((i*10))s log=$n wv2=$wv alerts=$al last=${st:-none}"
  case "$st" in *START_SIGNIN_WORKFLOW*) echo "SIGNIN_REACHED";; esac
done
echo SIGNIN_RUN_DONE
