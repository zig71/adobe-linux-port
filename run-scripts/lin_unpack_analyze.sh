#!/bin/bash
# Static analysis: what does CHECK_GENERAL_SYSTEM_REQUIREMENTS gate on?
# Unpack the UPX installer (copy) and look at the context around the
# 'not supported' message + which version APIs it imports. Zero OOM risk.
set -u
cd /home/kubuntu/Downloads
EXE=Creative_Cloud_Set-Up.exe
WORK=/home/kubuntu/adobe-wine-lab/work-unpack
mkdir -p "$WORK"
cp -f "$EXE" "$WORK/setup_packed.exe"
upx -d -o "$WORK/setup.exe" "$WORK/setup_packed.exe" >/dev/null 2>&1 || { echo UNPACK_FAIL; exit 1; }
ls -la "$WORK/setup.exe" | awk '{print "unpacked_bytes="$5}'

echo "=== 'not supported' string context ==="
grep -a -o -E '.{80}not supported.{120}' "$WORK/setup.exe" | head -5

echo
echo "=== nearby requirement/OS strings ==="
grep -a -o -E '(CHECK_GENERAL_SYSTEM_REQUIREMENTS|MinimumOS|MinOS|OSVersion|BuildNumber|DisplayVersion|CurrentBuild|EditionID|ProductName|GetProductInfo|VerifyVersionInfo|RtlGetVersion|IsWindows10OrGreater|137[0-9]{2}|1904[0-9]|22000|26100|26200)' "$WORK/setup.exe" | sort | uniq -c | sort -rn | head -30

echo
echo "=== imports of version APIs ==="
python3 /home/kubuntu/adobe-wine-lab/scripts/linux/pe_imports.py "$WORK/setup.exe" 2>/dev/null | grep -iE 'version|productinfo|rtlget|getversion|verify' | head -20 || echo "(pe_imports unavailable)"
echo UNPACK_DONE
