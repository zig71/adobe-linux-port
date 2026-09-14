#!/bin/bash
# Install Wine's Gecko add-on (the HTML engine backing mshtml) into the prefix.
#
# Wine needs gecko 2.47.4 for this build (dlls/mshtml/nsiface.idl). Without it,
# mshtml's HTMLDocument cannot be created: install_wine_gecko() raises a
# download prompt that blocks forever when there is no one to answer it, which
# is what makes CoCreateInstance(HKCR\MIME\...\text/html) hang.
set -e
W=/home/kubuntu/adobe-wine-lab/install/wine-arch/bin/wine
export WINEPREFIX=${WINEPREFIX:-/home/kubuntu/adobe-wine-lab/prefix-fresh}
export DISPLAY=:0
export XAUTHORITY=/run/user/1000/xauth_qmqmif
export WINEDEBUG=-all

DST=/home/kubuntu/adobe-wine-lab/gecko
mkdir -p "$DST"
cd "$DST"

VER=2.47.4
for arch in x86 x86_64; do
  f="wine-gecko-$VER-$arch.msi"
  if [ ! -f "$f" ]; then
    echo "== downloading $f =="
    curl -fsSL -o "$f" "https://dl.winehq.org/wine/wine-gecko/$VER/$f"
  fi
  ls -la "$f"
done

echo "== installing x86 mshtml gecko (32-bit) =="
timeout 600 "$W" msiexec /i "$DST/wine-gecko-$VER-x86.msi" /qn 2>&1 | tail -5
echo "MSI_X86 rc=$?"

echo "== installing x86_64 mshtml gecko =="
timeout 600 "$W" msiexec /i "$DST/wine-gecko-$VER-x86_64.msi" /qn 2>&1 | tail -5
echo "MSI_X64 rc=$?"

echo "== registry marker =="
"$W" reg query 'HKLM\Software\Wine\MSHTML\2.47.4' 2>&1 | head -8

echo "== gecko directories =="
ls "$WINEPREFIX/drive_c/windows/syswow64/gecko/" 2>&1 | head
ls "$WINEPREFIX/drive_c/windows/system32/gecko/" 2>&1 | head
find "$WINEPREFIX/drive_c/windows" -maxdepth 3 -name 'wine-gecko-*' -type d 2>/dev/null

echo GECKO_INSTALL_DONE
