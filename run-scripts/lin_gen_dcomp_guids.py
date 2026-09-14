#!/usr/bin/env python3
"""Generate dlls/dcomp/guids.c from the GUIDs the module references.

Including <initguid.h> turns every DEFINE_GUID in every transitively included
header into a definition, which then clashes with the copies in libuuid and
libdxguid. Defining only the GUIDs this module actually uses avoids that
entirely, and is the usual shape for a module that owns its own interfaces.

The values are read out of the generated headers so they cannot drift.
"""
import re
import pathlib
import sys

BUILD_INCLUDE = pathlib.Path("/home/kubuntu/adobe-wine-lab/src/wine/build-arch/include")
SRC = pathlib.Path("/home/kubuntu/adobe-wine-lab/src/wine/dlls/dcomp")

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

patterns = {n: re.compile(rf"DEFINE_GUID\(\s*{n}\s*,\s*([^)]*)\)", re.S) for n in NEEDED}
found = {}

for hdr in sorted(BUILD_INCLUDE.glob("*.h")):
    text = hdr.read_text(errors="replace")
    for name, pat in patterns.items():
        if name in found:
            continue
        m = pat.search(text)
        if m:
            vals = " ".join(m.group(1).split())
            found[name] = vals

missing = [n for n in NEEDED if n not in found]
if missing:
    print("NOT FOUND:", ", ".join(missing), file=sys.stderr)
    sys.exit(1)

HEAD = """/*
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

/* Only the GUIDs this module references are defined here, and they are written
 * out in full rather than via DEFINE_GUID under <initguid.h>: that would turn
 * every GUID of every transitively included header into a definition, clashing
 * with the copies in libuuid and libdxguid. */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"

"""
lines = [HEAD]
    "/*",
    " * DirectComposition interface GUIDs",
    " *",
    " * Copyright 2026 Adobe Wine Lab",
    " *",
    " * This library is free software; you can redistribute it and/or",
    " * modify it under the terms of the GNU Lesser General Public",
    " * License as published by the Free Software Foundation; either",
    " * version 2.1 of the License, or (at your option) any later version.",
    " *",
    " * This library is distributed in the hope that it will be useful,",
    " * but WITHOUT ANY WARRANTY; without even the implied warranty of",
    " * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU",
    " * Lesser General Public License for more details.",
    " *",
    " * You should have received a copy of the GNU Lesser General Public",
    " * License along with this library; if not, write to the Free Software",
    " * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA",
    " */",
    "",
    "/* Only the GUIDs this module references are defined here. Pulling in",
    " * <initguid.h> instead would define every GUID of every transitively",
    " * included header, clashing with libuuid and libdxguid. */",
    "",
    "#include <stdarg.h>",
    "",
    "#include \"windef.h\"",
    "#include \"winbase.h\"",
    "#include \"objbase.h\"",
    "#include \"dcomp.h\"",
    "",
]
for name in NEEDED:
    vals = [v.strip() for v in found[name].split(",")]
    assert len(vals) == 11, (name, vals)
    body = ", ".join(vals[3:])
    lines.append(f"const GUID {name} = {{ {vals[0]}, {vals[1]}, {vals[2]}, {{ {body} }} }};")
lines.append("")
(SRC / "guids.c").write_text("\n".join(lines) + "\n")
print(f"wrote guids.c with {len(NEEDED)} GUIDs")
