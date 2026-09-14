#!/usr/bin/env python3
"""Dump the CFG-related fields of a PE's load config directory.

Confirms which RVA holds __guard_dispatch_icall_fptr for a given image, so the
disassembly's `call DWORD PTR ds:<addr>` can be identified rather than guessed.
Only the fields up to GuardFlags are needed.
"""
import struct
import sys

# IMAGE_LOAD_CONFIG_DIRECTORY32 / 64 field offsets we care about
FIELDS32 = {
    0x3C: "SecurityCookie",
    0x48: "GuardCFCheckFunctionPointer",
    0x4C: "GuardCFDispatchFunctionPointer",
    0x50: "GuardCFFunctionTable",
    0x54: "GuardCFFunctionCount",
    0x58: "GuardFlags",
}
FIELDS64 = {
    0x58: "SecurityCookie",
    0x70: "GuardCFCheckFunctionPointer",
    0x78: "GuardCFDispatchFunctionPointer",
    0x80: "GuardCFFunctionTable",
    0x88: "GuardCFFunctionCount",
    0x90: "GuardFlags",
}
GUARD_FLAGS = {
    0x00000100: "CF_INSTRUMENTED",
    0x00000200: "CFW_INSTRUMENTED",
    0x00000400: "CF_FUNCTION_TABLE_PRESENT",
    0x00000800: "SECURITY_COOKIE_UNUSED",
    0x00001000: "PROTECT_DELAYLOAD_IAT",
    0x00002000: "DELAYLOAD_IAT_IN_ITS_OWN_SECTION",
    0x00004000: "CF_EXPORT_SUPPRESSION_INFO_PRESENT",
    0x00008000: "CF_ENABLE_EXPORT_SUPPRESSION",
    0x00010000: "CF_LONGJUMP_TABLE_PRESENT",
    0x00040000: "EH_CONTINUATION_TABLE_PRESENT",
}


def rva_to_off(rva, sections):
    for va, vsz, raw, rawsz in sections:
        if va <= rva < va + max(vsz, rawsz):
            return raw + (rva - va)
    return None


def main(path):
    data = open(path, "rb").read()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    machine, nsec = struct.unpack_from("<HH", data, pe + 4)
    optsz = struct.unpack_from("<H", data, pe + 16)[0]
    opt = pe + 20
    magic = struct.unpack_from("<H", data, opt)[0]
    is64 = magic == 0x20B
    fields = FIELDS64 if is64 else FIELDS32
    dd = opt + (112 if is64 else 96)

    # data directory 10 (index 10 = LOAD_CONFIG), entries are 8 bytes each
    lc_rva, lc_size = struct.unpack_from("<II", data, dd + 10 * 8)

    sec = opt + optsz
    sections = []
    for i in range(nsec):
        b = sec + i * 40
        vsz, va, rawsz, raw = struct.unpack_from("<IIII", data, b + 8)
        sections.append((va, vsz, raw, rawsz))

    print(f"ARCH={'x86_64' if is64 else 'i386'}")
    print(f"MACHINE=0x{machine:04x}")
    print(f"LOAD_CONFIG_RVA=0x{lc_rva:x} SIZE={lc_size}")
    if not lc_rva:
        print("NO LOAD CONFIG")
        return

    off = rva_to_off(lc_rva, sections)
    size = struct.unpack_from("<I", data, off)[0]
    print(f"Size_field={size}")
    for fo, name in sorted(fields.items()):
        if fo + 4 > (lc_size or size):
            continue
        v = struct.unpack_from("<I", data, off + fo)[0]
        note = ""
        if name == "GuardFlags":
            bits = " | ".join(n for b, n in GUARD_FLAGS.items() if v & b)
            note = f"  ({bits})" if bits else "  (none)"
        print(f"{name:34} = 0x{v:08x}{note}")


if __name__ == "__main__":
    main(sys.argv[1])
