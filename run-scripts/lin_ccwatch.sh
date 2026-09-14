#!/bin/bash
# Watch a fresh CC launch: child census + capture any mapped window live.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-watch2.out 2>&1 < /dev/null &
for i in $(seq 1 12); do
  sleep 15
  gpu=$(pgrep -af '[U]I Helper' 2>/dev/null | grep -ac 'type=gpu-process')
  rnd=$(pgrep -af '[U]I Helper' 2>/dev/null | grep -ac 'type=renderer')
  echo "t=$((i*15))s gpu=$gpu renderer=$rnd"
  WIN=$(xwininfo -root -children 2>/dev/null | grep -a 'AXWIN Frame Window' | grep -aoE '0x[0-9a-f]+' | head -1)
  if [ -n "$WIN" ]; then
    st=$(xwininfo -id "$WIN" 2>/dev/null | grep 'Map State' | grep -aoE 'Is[A-Za-z]+')
    echo "axwin=$WIN state=$st"
    if [ "$st" = "IsViewable" ]; then
      spectacle -b -n -o /tmp/cc-live.png 2>/dev/null; ls -la /tmp/cc-live.png 2>/dev/null; echo CAPTURED; break
    fi
  fi
done
echo WATCH_DONE
