#!/bin/bash
# Measure where this guest's memory goes, so the OOM fix is informed.
#
# The Creative Cloud installer with the real WebView2 runtime pulls in Wine plus
# several Chromium processes including a 332 MB msedge.dll; on a 5.9 GB guest
# that trips the global OOM killer, which then kills session processes and takes
# Plasma and Xwayland down with it.
set -u

echo "=== memory now ==="
free -m
echo
echo "=== top 20 by RSS ==="
ps -eo rss,pid,user,comm --sort=-rss 2>/dev/null | head -21 | awk '{printf "%8.1f MB  %-7s %-8s %s\n", $1/1024, $2, $3, $4}'
echo
echo "=== memory by process group (MB) ==="
for pat in plasmashell kwin kded kaccess kdeconnect ksecret plasma Xwayland wineserver msedgewebview2 'Creative_Cloud' Xvfb; do
    tot=$(ps -eo rss,comm 2>/dev/null | grep -i "$pat" | awk '{s+=$1} END {print s+0}')
    printf '%-20s %8.1f MB\n' "$pat" "$(echo "scale=1; $tot/1024" | bc)"
done
echo
echo "=== swap ==="
swapon --show 2>/dev/null || echo "(no swap beyond default)"
echo
echo "=== /dev/shm (Chromium needs this) ==="
df -h /dev/shm | tail -1
echo
echo "=== total RAM ==="
awk '/MemTotal/{printf "%.0f MB\n", $2/1024}' /proc/meminfo
echo MEM_MEASURE_DONE
