#!/bin/bash
# The generated dcomp.h defines IDCompositionDevice3_Commit(This) etc. as
# macros for every interface method. Generated implementations that carry the
# same names get macro-expanded into nonsense, so the implementations are
# renamed to a distinct prefix while the header macros remain usable for calls
# into other interfaces.
set -eu
cd /home/kubuntu/adobe-wine-lab/src/wine/dlls/dcomp

echo "== device.c =="
sed -i -E \
  -e 's/#define GEN_DEVICE1\(IF, CONV\)/#define GEN_DEVICE1(NAME, IF, CONV)/' \
  -e 's/#define GEN_DEVICE2_CORE\(IF, CONV\)/#define GEN_DEVICE2_CORE(NAME, IF, CONV)/' \
  -e 's/#define GEN_DEVICE3_EXTRA\(IF\)/#define GEN_DEVICE3_EXTRA(NAME, IF)/' \
  -e 's/#define GEN_DESKTOP_EXTRA\(IF, CONV\)/#define GEN_DESKTOP_EXTRA(NAME, IF, CONV)/' \
  -e 's/#define GEN_IUNKNOWN\(IF, CONV\)/#define GEN_IUNKNOWN(NAME, IF, CONV)/' \
  -e 's/IF##_/NAME##_/g' \
  device.c
sed -i -E \
  -e 's/GEN_DEVICE1\(IDCompositionDevice,/GEN_DEVICE1(dcomp_device1, IDCompositionDevice,/' \
  -e 's/GEN_DEVICE2_CORE\(IDCompositionDevice3,/GEN_DEVICE2_CORE(dcomp_device3, IDCompositionDevice3,/' \
  -e 's/GEN_DEVICE3_EXTRA\(IDCompositionDevice3\)/GEN_DEVICE3_EXTRA(dcomp_device3, IDCompositionDevice3)/' \
  -e 's/GEN_DEVICE2_CORE\(IDCompositionDesktopDevice,/GEN_DEVICE2_CORE(dcomp_desktop, IDCompositionDesktopDevice,/' \
  -e 's/GEN_DESKTOP_EXTRA\(IDCompositionDesktopDevice,/GEN_DESKTOP_EXTRA(dcomp_desktop, IDCompositionDesktopDevice,/' \
  -e 's/GEN_IUNKNOWN\(IDCompositionDevice,/GEN_IUNKNOWN(dcomp_device1, IDCompositionDevice,/' \
  -e 's/GEN_IUNKNOWN\(IDCompositionDevice3,/GEN_IUNKNOWN(dcomp_device3, IDCompositionDevice3,/' \
  -e 's/GEN_IUNKNOWN\(IDCompositionDesktopDevice,/GEN_IUNKNOWN(dcomp_desktop, IDCompositionDesktopDevice,/' \
  -e 's/\bIDCompositionDevice3_([A-Z])/dcomp_device3_\1/g' \
  -e 's/\bIDCompositionDesktopDevice_([A-Z])/dcomp_desktop_\1/g' \
  -e 's/\bIDCompositionDevice_([A-Z])/dcomp_device1_\1/g' \
  device.c

echo "== target.c =="
sed -i -E \
  -e 's/#define GEN_VISUAL\(IF, CONV\)/#define GEN_VISUAL(NAME, IF, CONV)/' \
  -e 's/#define GEN_VISUAL_V2\(IF, CONV\)/#define GEN_VISUAL_V2(NAME, IF, CONV)/' \
  -e 's/#define GEN_VISUAL2_EXTRA\(IF, CONV\)/#define GEN_VISUAL2_EXTRA(NAME, IF, CONV)/' \
  -e 's/IF##_/NAME##_/g' \
  target.c
sed -i -E \
  -e 's/GEN_VISUAL\(IDCompositionVisual,/GEN_VISUAL(dcomp_visual1, IDCompositionVisual,/' \
  -e 's/GEN_VISUAL_V2\(IDCompositionVisual2,/GEN_VISUAL_V2(dcomp_visual2, IDCompositionVisual2,/' \
  -e 's/GEN_VISUAL2_EXTRA\(IDCompositionVisual2,/GEN_VISUAL2_EXTRA(dcomp_visual2, IDCompositionVisual2,/' \
  -e 's/\bIDCompositionVisual2_([A-Z])/dcomp_visual2_\1/g' \
  -e 's/\bIDCompositionVisual_([A-Z])/dcomp_visual1_\1/g' \
  target.c

echo "== surface.c =="
sed -i -E \
  -e 's/#define GEN_SURFACE\(IF, CONV\)/#define GEN_SURFACE(NAME, IF, CONV)/' \
  -e 's/#define GEN_VIRTUAL_SURFACE\(IF, CONV\)/#define GEN_VIRTUAL_SURFACE(NAME, IF, CONV)/' \
  -e 's/IF##_/NAME##_/g' \
  surface.c
sed -i -E \
  -e 's/GEN_SURFACE\(IDCompositionSurface,/GEN_SURFACE(dcomp_surface1, IDCompositionSurface,/' \
  -e 's/GEN_VIRTUAL_SURFACE\(IDCompositionVirtualSurface,/GEN_VIRTUAL_SURFACE(dcomp_vsurface, IDCompositionVirtualSurface,/' \
  -e 's/\bIDCompositionVirtualSurface_([A-Z])/dcomp_vsurface_\1/g' \
  -e 's/\bIDCompositionSurface_([A-Z])/dcomp_surface1_\1/g' \
  surface.c

echo "== sanity =="
for f in device.c target.c surface.c; do
  echo "--- $f"
  grep -nE 'GEN_[A-Z0-9_]+_(Commit|QueryInterface),' "$f" | head -4
  grep -cE '^([A-Za-z_ ]*)(IDComposition[A-Za-z0-9]*)_(Commit|QueryInterface|BeginDraw|SetOffsetX)\(' "$f" || true
done
echo RENAMES_DONE
