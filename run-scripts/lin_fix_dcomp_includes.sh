#!/bin/bash
# Each source included dcomp.h directly before dcomp_private.h, so COBJMACROS
# was not yet defined and none of the IDComposition*_Method() wrappers existed.
# COBJMACROS now has to be set before d3d11.h/dxgi.h/dcomp.h, and the direct
# dcomp.h includes are removed so the private header is the single entry point.
# initguid.h stays in device.c so the interface GUIDs are defined exactly once.
set -eu
D=/home/kubuntu/adobe-wine-lab/src/wine/dlls/dcomp
cd "$D"

python3 - <<'PY'
import re, pathlib

d = pathlib.Path("/home/kubuntu/adobe-wine-lab/src/wine/dlls/dcomp")
h = d / "dcomp_private.h"
s = h.read_text()

s = s.replace(
'''#include "d3d11.h"
#include "dxgi.h"

/* The generated dcomp.h only provides the IDComposition*_Method() wrappers
 * when COBJMACROS is defined. */
#define COBJMACROS
#include "dcomp.h"''',
'''/* The generated headers only provide the Interface_Method() wrappers when
 * COBJMACROS is defined, so it has to precede all of them. */
#define COBJMACROS
#include "d3d11.h"
#include "dxgi.h"
#include "dcomp.h"''')
h.write_text(s)
print("header rewritten")

for name in ("device.c", "target.c", "surface.c", "transform.c", "misc.c"):
    p = d / name
    s = p.read_text()
    before = s
    # Drop direct dcomp.h includes; the private header provides it.
    s = s.replace('#include "dcomp.h"\n\n', '')
    # Drop now-redundant initguid includes outside device.c.
    if name != "device.c":
        s = s.replace('#include "initguid.h"\n', '')
    if s == before:
        print(f"{name}: no change")
    else:
        p.write_text(s)
        print(f"{name}: updated")
PY

echo "== includes now =="
for f in device.c target.c surface.c transform.c misc.c; do
  echo "--- $f"
  grep -n '#include' "$f" | head -12
done
