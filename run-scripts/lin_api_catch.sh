#!/bin/bash
# API-catch run: which version API does the check call, with what result?
# +relay filtered live to version APIs only; Chromium children reaped on
# sight (the gate runs in the installer long before any page loads).
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=+relay,+tid,+pid,+timestamp
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }

OUT=/tmp/cc-apis.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null; sleep 2
rm -f "$OUT" "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe 2>&1 < /dev/null \
  | grep -a -E 'GetProductInfo|RtlGetVersion|RtlGetNtVersionNumbers|VerifyVersionInfo|RtlVerifyVersionInfo|GetVersionEx|IsWindows|NtQuerySystemInformation|GlobalMemoryStatus|GetSystemInfo|GetNativeSystemInfo|GetDiskFreeSpace|IsWow64|GetComputerName|GetSystemMetrics' \
  > "$OUT" &
SECS=${1:-90}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  pkill -9 -f 'msedgewebview2' 2>/dev/null || true
  avail=$(free -m | awk '/^Mem:/{print $7}')
  alert=$(grep -a -c 'showErrorAlert' "$LOG" 2>/dev/null || echo 0)
  echo "t=$((i*10))s avail=${avail}MB alert=$alert outkb=$(du -k "$OUT" 2>/dev/null | cut -f1)"
  if [ "$alert" != "0" ] && [ "$alert" != "" ]; then
    echo "ALERT_LOGGED, waiting 8s then killing"
    sleep 8
    break
  fi
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview2' 2>/dev/null
sleep 1
echo "=== WAM ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -aE 'showErrorAlert|not supported|state: ' | cut -c1-130 | head -8
echo "=== out ==="
ls -la "$OUT"
echo APICATCH_DONE
