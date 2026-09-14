#!/bin/bash
# Determine why Xwayland restarted during the installer run.
#
# The installer's Chromium processes were alive and had reached a gpu-process,
# but the run was cut short by "X connection to :0 broken" and Xwayland came
# back with a new auth cookie and PID, i.e. it was restarted during the run.
# That has to be understood before any of the installer's later stages can be
# judged, because a dead X connection stops Wine creating windows entirely.
set -u

echo "=== Xwayland history this boot ==="
journalctl --user -b --no-pager -n 200 2>/dev/null | grep -iE 'xwayland|plasma|kwin|segfault|crash' | tail -30 \
  || echo "(no user journal)"

echo
echo "=== system journal, Xwayland / crashes ==="
sudo -n journalctl -b --no-pager -n 400 2>/dev/null | grep -iE 'xwayland|segfault|oom|killed process|kwin' | tail -30 \
  || echo "(needs sudo or no entries)"

echo
echo "=== kernel messages about kills / OOM ==="
sudo -n dmesg 2>/dev/null | grep -iE 'oom|killed|segfault|xwayland' | tail -20 || echo "(no dmesg access)"

echo
echo "=== crash artefacts ==="
ls -1t /var/crash/ 2>/dev/null | head -5 || echo "(none)"
coredumpctl list --no-pager 2>/dev/null | tail -10 || echo "(no coredumpctl entries)"

echo
echo "=== memory pressure during the run ==="
free -m | head -2
cat /proc/pressure/memory 2>/dev/null || echo "(no psi)"

echo
echo "=== Xwayland start time vs now ==="
ps -o pid,lstart,etime,cmd -p "$(pgrep -f 'Xwayland :0' | head -1)" 2>/dev/null

echo
echo "=== stray Chromium / Wine processes to clean ==="
pgrep -af 'msedgewebview2' | wc -l
pgrep -af 'wineserver|\.exe' | head -8
echo XDIAG_DONE
