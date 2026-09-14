#!/usr/bin/env python3
"""Emit dlls/dcomp/guids.c: explicit definitions for the GUIDs this module uses.

<initguid.h> is not used: it turns every DEFINE_GUID of every transitively
included header into a definition, which clashes with the copies already in
libuuid and libdxguid. Writing the handful of GUIDs out in full avoids that and
needs no UUID import at all.

Values are read from the generated headers so they cannot drift.
"""
import re
import pathlib
import sys

BUILD_INCLUDE = pathlib.Path("/home/kubuntu/adobe-wine-lab/src/wine/build-arch/include")
OUT = pathlib.Path("/home/kubuntu/adobe-wine-lab/src/wine/dlls/dcomp/guids.c")

NEEDED = [
    "IID_IUnknown",
    "IID_IDCompositionDevice", "IID_IDCompositionDevice2", "IID_IDCompositionDevice3",
    "IID_IDCompositionDesktopDevice", "IID_IDCompositionTarget",
    "IID_IDCompositionVisual", "IID_IDCompositionVisual2",
    "IID_IDCompositionSurface", "IID_IDCompositionVirtualSurface",
    "IID_IDCompositionSurfaceFactory", "IID_IDCompositionAnimation",
    "IID_IDCompositionEffectGroup", "IID_IDCompositionRectangleClip",
    "IID_IDCompositionClip", "IID_IDCompositionEffect",
    "IID_IDCompositionTransform", "IID_IDCompositionTransform3D",
    "IID_IDCompositionTranslateTransform", "IID_IDCompositionScaleTransform",
    "IID_IDCompositionMatrixTransform", "IID_IDCompositionSkewTransform",
    "IID_IDCompositionRotateTransform",
    "IID_ID3D11Device", "IID_IDXGIDevice", "IID_IDXGISurface",
]

found = {}
for hdr in sorted(BUILD_INCLUDE.glob("*.h")):
    text = hdr.read_text(errors="replace")
    for name in NEEDED:
        if name in found:
            continue
        m = re.search(rf"DEFINE_GUID\(\s*{name}\s*,\s*([^)]*)\)", text, re.S)
        if m:
            found[name] = " ".join(m.group(1).split())

missing = [n for n in NEEDED if n not in found]
if missing:
    print("NOT FOUND: " + ", ".join(missing), file=sys.stderr)
    sys.exit(1)

LICENCE = """/*
 * DirectComposition interface GUIDs
 *
 * Copyright 2026 Adobe Wine Lab
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
"""

body = [
    LICENCE,
    "/* Only the GUIDs this module references are defined, and they are written out in",
    " * full rather than through DEFINE_GUID under <initguid.h>: that would define every",
    " * GUID of every transitively included header, clashing with libuuid and libdxguid. */",
    "",
    "#include <stdarg.h>",
    "",
    '#include "windef.h"',
    '#include "winbase.h"',
    '#include "objbase.h"',
    "",
]

for name in NEEDED:
    vals = [v.strip() for v in found[name].split(",")]
    if len(vals) != 11:
        print(f"{name}: unexpected value count {len(vals)}", file=sys.stderr)
        sys.exit(1)
    tail = ", ".join(vals[3:])
    body.append(f"const GUID {name} = {{ {vals[0]}, {vals[1]}, {vals[2]}, {{ {tail} }} }};")

OUT.write_text("\n".join(body) + "\n")
print(f"wrote {OUT} with {len(NEEDED)} GUIDs")
