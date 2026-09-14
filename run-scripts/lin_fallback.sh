#!/bin/bash
# Fallback-path test: hide the WebView2 runtime so the installer takes its
# IE/mshtml path, with all current Wine fixes in place. Restores afterwards.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2

echo "=== hiding runtime ==="
"$W" reg export 'HKLM\Software\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}' /tmp/clients_backup.reg 2>&1 | tail -1
"$W" reg delete 'HKLM\Software\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}' /f 2>&1 | tail -1
mv ~/adobe-wine-lab/prefix-wv2/drive_c/Program\ Files\ \(x86\)/Microsoft/EdgeWebView ~/adobe-wine-lab/EdgeWebView_hidden 2>&1
ls ~/adobe-wine-lab/ | grep -i hidden

OUT=/tmp/cc-fallback.out
LOG="$WINEPREFIX/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/WAM.log"
rm -f "$OUT" "$LOG"

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > "$OUT" 2>&1 < /dev/null &
SECS=${1:-180}
for i in $(seq 1 $((SECS / 10))); do
  sleep 10
  avail=$(free -m | awk '/^Mem:/{print $7}')
  n=$(wc -l < "$LOG" 2>/dev/null || echo 0)
  wv=$(pgrep -c -f 'msedgewebview[2]' 2>/dev/null || echo 0)
  st=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | tail -1)
  rs=$(tr -d '\000' < "$LOG" 2>/dev/null | grep -ac 'onWindowResize\|NavigateComplete')
  echo "t=$((i*10))s log=$n wv2=$wv avail=${avail}MB resize=$rs last=${st:-none}"
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
xwd -root -silent 2>/dev/null | convert xwd:- /tmp/fb-final.png 2>/dev/null || true
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
sleep 1

echo "=== restoring runtime ==="
mv ~/adobe-wine-lab/EdgeWebView_hidden ~/adobe-wine-lab/prefix-wv2/drive_c/Program\ Files\ \(x86\)/Microsoft/EdgeWebView
"$W" reg import /tmp/clients_backup.reg 2>&1 | tail -1
"$W" reg query 'HKLM\Software\WOW6432Node\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}' /v pv 2>/dev/null

echo "=== verdict ==="
tr -d '\000' < "$LOG" 2>/dev/null | grep -a 'not supported' | cut -c1-120 | head -2 || echo "(no not-supported)"
tr -d '\000' < "$LOG" 2>/dev/null | grep -aoE 'state: [A-Z_]+' | sort -u
echo "(end)"
echo FALLBACK_DONE
