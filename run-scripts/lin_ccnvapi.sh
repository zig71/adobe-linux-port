#!/bin/bash
# NvAPI-hack-off test: real adapter ID to defeat the GPU blocklist.
set -u
export WINEPREFIX=/home/kubuntu/adobe-wine-lab/prefix-wv2
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_yhSBnD
export WINEDEBUG=-all
export DXVK_CONFIG_FILE=/home/kubuntu/adobe-wine-lab/dxvk-lvp.conf
export DXVK_FILTER_DEVICE_NAME=llvmpipe
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
pkill -9 -f 'Creative Clou[d]' 2>/dev/null; sleep 2
cd /home/kubuntu
setsid "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' > /tmp/cc-nvapi.out 2>&1 < /dev/null &
sleep 25
timeout 40 "$W" 'C:\Program Files (x86)\Adobe\Adobe Creative Cloud\ACC\Creative Cloud.exe' >> /tmp/cc-nvapi.out 2>&1
sleep 60
echo "=== CEF tail ==="
tail -6 ~/adobe-wine-lab/prefix-wv2/drive_c/users/kubuntu/AppData/Local/Temp/CreativeCloud/ACC/CEF.log 2>/dev/null | grep -aE 'crashed|FATAL|backing|mailbox|GPU process' | cut -c1-120 | head -6
echo "=== axwin ==="
WIN=$(xwininfo -root -children 2>/dev/null | grep -a 'AXWIN Frame Window' | grep -aoE '0x[0-9a-f]+' | head -1)
echo "axwin=$WIN"
if [ -n "$WIN" ]; then
  st=$(xwininfo -id "$WIN" 2>/dev/null | grep 'Map State' | grep -aoE 'Is[A-Za-z]+')
  echo "state=$st"
  if [ "$st" = "IsViewable" ]; then
    sleep 15
    spectacle -b -n -o /tmp/cc-nvapi.png 2>/dev/null; ls -la /tmp/cc-nvapi.png 2>/dev/null
  fi
fi
echo NVAPI_DONE
