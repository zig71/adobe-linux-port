#!/bin/bash
# Identify the registry key the installer fails to open under Wine.
#
# The installer logs, immediately before it stalls:
#     WARN | OSUtils | getRegistryValue: RegOpenKeyExW failed with error 2
# whereas on the Windows oracle the same step logs
#     WARN | OSUtils | getRegistryValue: RegQueryValueExW failed with error 2
# i.e. on Windows the key opens and a value is absent; under Wine the key
# itself is missing. This traces registry access to name the key.
set -u
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-fresh}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
INSTALLER=/home/kubuntu/Downloads/Creative_Cloud_Set-Up.exe
SECS=${1:-120}
OUT=/tmp/regtrace.out

pkill -9 -f 'Set-Up' 2>/dev/null
sleep 2
rm -f "$OUT"

cd /home/kubuntu/Downloads
WINEDEBUG=+reg timeout "$SECS" "$W" "$INSTALLER" > "$OUT" 2>&1
echo "lines=$(wc -l < "$OUT")"

echo "=== failing registry operations (non-zero status) ==="
grep -aE 'err:reg:|:reg:Nt(Open|Query)' "$OUT" | grep -aiE 'c0000034|OBJECT_NAME_NOT_FOUND|0xc0000034|not found|status=' | tail -25

echo "=== last registry keys touched before the stall ==="
grep -aE 'trace:reg:NtOpenKey' "$OUT" | tail -30

echo "=== distinct key names touched (tail) ==="
grep -aoE 'L"[^"]{3,}"' "$OUT" | sort | uniq -c | sort -rn | head -30
echo REGTRACE_DONE
