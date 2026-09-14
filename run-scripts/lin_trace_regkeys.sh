#!/bin/bash
# Name the registry keys the installer opens, and which of those fail.
#
# The installer logs "getRegistryValue: RegOpenKeyExW failed with error 2"
# under Wine but "RegQueryValueExW failed with error 2" on the Windows oracle,
# so under Wine a key is missing that exists on Windows. The generic +reg
# channel is dominated by crypto OID lookups, so trace the entry point itself.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-fresh}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
INSTALLER=/home/kubuntu/Downloads/Creative_Cloud_Set-Up.exe
SECS=${1:-110}
OUT=/tmp/regrelay.out

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$OUT"

cd /home/kubuntu/Downloads
WINEDEBUG=+relay timeout "$SECS" "$W" "$INSTALLER" 2>&1 \
  | grep -aE 'RegOpenKeyExW|RegQueryValueExW|RegGetValueW' \
  > "$OUT"

echo "lines=$(wc -l < "$OUT")"
echo "=== every RegOpenKeyExW call with its key ==="
grep -aE 'Call .*RegOpenKeyExW' "$OUT" | sed -E 's/^[0-9a-f]+:Call //; s/ret=[0-9a-f]+$//' | sort -u | head -60
echo
echo "=== key opens that failed (Ret non-zero) ==="
paste -d'|' \
  <(grep -aE 'Call .*RegOpenKeyExW' "$OUT" | sed -E 's/^[0-9a-f]+:Call //') \
  <(grep -aE 'Ret  .*RegOpenKeyExW' "$OUT" | sed -E 's/^[0-9a-f]+:Ret  //') 2>/dev/null \
  | grep -avE 'retval=00000000' | head -30
echo
echo "=== RegQueryValueExW calls that failed ==="
grep -aE 'Ret  .*RegQueryValueExW' "$OUT" | grep -av 'retval=00000000' | head -20
echo REGRELAY_DONE
