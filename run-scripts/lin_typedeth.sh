#!/bin/bash
# Per-type mortality: 1s census with process-type capture; report which
# browser process types die.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:99
unset XAUTHORITY
export WINEDEBUG=-all
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine

timeout 10 bash -c 'DISPLAY=:99 xdpyinfo >/dev/null' || { echo XVFB_DEAD; exit 1; }
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null; sleep 2
rm -f /tmp/cc-type.out

cd /home/kubuntu/Downloads
setsid "$W" ./Creative_Cloud_Set-Up.exe > /tmp/cc-type.out 2>&1 < /dev/null &
prev=""
for i in $(seq 1 90); do
  sleep 1
  cur=$(pgrep -af 'msedgewebview[2]' | grep -a -o -E '^[0-9]+ .*--type=[a-z-]+|^[0-9]+ .*--embedded-browser' | sort)
  if [ "$i" -gt "1" ] && [ -n "$prev" ]; then
    if [ -s /tmp/died.txt ]; then
      echo "t=${i}s DEATHS:"
      while read -r p; do
        echo "$prev" | grep -a "^$p " | cut -c1-140
      done < /tmp/died.txt
    fi
  fi
  prev="$cur"
  avail=$(free -m | awk '/^Mem:/{print $7}')
  if [ "$avail" -lt 400 ]; then echo "MEMORY GUARD TRIPPED"; break; fi
done
pkill -9 -f 'Set-Up' 2>/dev/null; pkill -9 -f 'msedgewebview[2]' 2>/dev/null
echo TYPEDONE
