#!/bin/bash
# Establish what killed the Plasma session during the installer run, and what
# headroom exists for the workload.
set -u

echo "=== kernel OOM / kill messages ==="
echo ${LAB_PW} | sudo -S -p '' dmesg 2>/dev/null | grep -iE 'oom|killed process|out of memory|segfault' | tail -25 \
  || echo "(no dmesg entries or no access)"

echo
echo "=== memory pressure now ==="
free -m
echo "--- cgroup memory limits ---"
cat /sys/fs/cgroup/user.slice/user-1000.slice/memory.max 2>/dev/null || echo "(n/a)"
cat /sys/fs/cgroup/memory.max 2>/dev/null || echo "(n/a)"

echo
echo "=== available headless X servers ==="
for t in Xvfb Xephyr xvfb-run weston Xorg; do
  p=$(command -v "$t" 2>/dev/null)
  printf '%-12s %s\n' "$t" "${p:-NOT INSTALLED}"
done

echo
echo "=== screenshot tooling that works headlessly ==="
for t in xwd import convert ffmpeg xdotool wmctrl; do
  p=$(command -v "$t" 2>/dev/null)
  printf '%-12s %s\n' "$t" "${p:-NOT INSTALLED}"
done

echo
echo "=== desktop session ==="
echo "XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-unset} WAYLAND_DISPLAY=${WAYLAND_DISPLAY:-unset}"
pgrep -a plasmashell | head -2
pgrep -a kwin_wayland | head -2

echo
echo "=== vCPU count and total RAM (workload sizing) ==="
nproc
echo "MemTotal: $(awk '/MemTotal/{print $2/1024 " MB"}' /proc/meminfo)"
echo DIAG2_DONE
