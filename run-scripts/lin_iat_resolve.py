#!/usr/bin/env python3
"""Resolve IAT slot file-offset 0x137110A0 to (DLL, function) via the PE
import directory. Read-only."""
import struct
import sys

F = "/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll"
SLOT = 0x137110A0

d = open(F, "rb").read()
pe = struct.unpack_from("<I", d, 0x3C)[0]
assert d[pe:pe + 4] == b"PE\0\0"
nsec = struct.unpack_from("<H", d, pe + 6)[0]
opth = pe + 24
magic = struct.unpack_from("<H", d, opth)[0]
assert magic == 0x20B, hex(magic)
dd = opth + 112  # data directories (PE32+)
dd = opth + 92  # PE32+: 24 std + 68 windows fields, then directories
imp_rva, imp_size = struct.unpack_from("<II", d, dd + 8)  # [1] = Import Table


def rva_to_off(rva):
    off = pe + 24 + struct.unpack_from("<H", d, pe + 20)[0]
    for _ in range(nsec):
        vsz, vaddr, rawsz, raw = struct.unpack_from("<IIII", d, off + 8)
        if vaddr <= rva < vaddr + max(vsz, rawsz):
            return raw + (rva - vaddr)
        off += 40
    return None


def cstr(off):
    end = d.index(b"\0", off)
    return d[off:end].decode("ascii", "replace")


off = rva_to_off(imp_rva)
n = 0
while True:
    ilt, ts, fw, name_rva, iat = struct.unpack_from("<IIIII", d, off)
    if ilt == 0 and name_rva == 0 and iat == 0:
        break
    dll = cstr(rva_to_off(name_rva))
    # walk IAT
    io = rva_to_off(iat)
    idx = 0
    while True:
        thunk = struct.unpack_from("<Q", d, io)[0]
        if thunk == 0:
            break
        # IAT slot RVA for this entry:
        if iat + idx * 8 == SLOT:
            if thunk & (1 << 63):
                print(f"HIT {dll} ordinal {thunk & 0xFFFF}")
            else:
                hint_name_off = rva_to_off(thunk & 0x7FFFFFFF)
                hint = struct.unpack_from("<H", d, hint_name_off)[0]
                print(f"HIT {dll} ! {cstr(hint_name_off + 2)} (hint {hint})")
            sys.exit(0)
        io += 8
        idx += 1
        if idx > 20000:
            break
    off += 20
    n += 1
    if n > 1000:
        break
print("MISS")
