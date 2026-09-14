#!/usr/bin/env python3
"""List the DLLs a PE image imports, without loading it.

Used to name the missing dependency that makes Wine fail to load
msedge.dll (the Edge WebView2 Chromium core) with ERROR_MOD_NOT_FOUND (126)
while the smaller shim DLLs load successfully.
"""
import struct
import sys


def rva_to_off(rva, sections):
    for va, vsz, raw, rawsz in sections:
        if va <= rva < va + max(vsz, rawsz):
            return raw + (rva - va)
    return None


def cstr(data, off):
    end = data.index(b"\0", off)
    return data[off:end].decode("ascii", "replace")


def imports(path):
    with open(path, "rb") as f:
        data = f.read()

    if data[:2] != b"MZ":
        raise SystemExit("not a PE file")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise SystemExit("bad PE signature")

    coff = pe + 4
    machine, nsec = struct.unpack_from("<HH", data, coff)
    optsz = struct.unpack_from("<H", data, coff + 16)[0]
    opt = coff + 20
    magic = struct.unpack_from("<H", data, opt)[0]
    is_pe32plus = magic == 0x20B

    # Data directories: PE32+ starts at opt+112, PE32 at opt+96
    dd = opt + (112 if is_pe32plus else 96)
    imp_rva, imp_sz = struct.unpack_from("<II", data, dd + 8)  # entry 1 = imports

    sec = opt + optsz
    sections = []
    for i in range(nsec):
        base = sec + i * 40
        vsz, va, rawsz, raw = struct.unpack_from("<IIII", data, base + 8)
        sections.append((va, vsz, raw, rawsz))

    off = rva_to_off(imp_rva, sections)
    if off is None:
        return machine, []

    names = []
    i = 0
    while True:
        base = off + i * 20
        entry = data[base:base + 20]
        if len(entry) < 20 or entry == b"\0" * 20:
            break
        name_rva = struct.unpack_from("<I", entry, 12)[0]
        if name_rva == 0:
            break
        noff = rva_to_off(name_rva, sections)
        names.append(cstr(data, noff) if noff is not None else "?")
        i += 1
        if i > 512:
            break

    return machine, names


if __name__ == "__main__":
    m, names = imports(sys.argv[1])
    arch = {0x14C: "i386", 0x8664: "x86_64", 0xAA64: "arm64"}.get(m, hex(m))
    print(f"ARCH={arch}")
    print(f"IMPORT_COUNT={len(names)}")
    for n in sorted(set(names)):
        print(f"IMPORT={n}")
