#!/usr/bin/env python3
"""Minidump triage, two passes: modules+exception first, then the crashing
thread's context and stack as module-relative offsets (ASLR-proof)."""
import struct
import sys
import glob

HDR = struct.Struct("<4sI I I I I Q")
DIR = struct.Struct("<I I I")
EXC_STREAM = struct.Struct("<I I")
EXC_REC = struct.Struct("<I I Q Q Q I")
MODENT = struct.Struct("<Q I I I I")

EXC_NAMES = {
    0xC0000005: "ACCESS_VIOLATION",
    0xC000001D: "ILLEGAL_INSTRUCTION",
    0xC0000094: "INT_DIVIDE_BY_ZERO",
    0xC00000FD: "STACK_OVERFLOW",
    0x80000003: "BREAKPOINT",
    0xC0000409: "STACK_BUFFER_OVERRUN",
    0xE06D7363: "CPP_EXCEPTION",
}


def modname(data, rva):
    nlen = struct.unpack_from("<I", data, rva)[0]
    raw = data[rva + 4:rva + 4 + nlen]
    name = raw.decode("utf-16-le", "replace")
    return name.replace("\\", "/").split("/")[-1]


WANT = ("msedge.dll", "msedgewebview2.exe", "oneauth.dll", "d3d11.dll",
        "dxgi.dll", "dcomp.dll", "DWrite.dll", "ntdll.dll", "kernelbase.dll")

for path in sorted(glob.glob(sys.argv[1] if len(sys.argv) > 1 else "*.dmp")):
    with open(path, "rb") as f:
        data = f.read()
    sig, ver, nstreams, dirstart, _, _, _ = HDR.unpack_from(data, 0)
    if sig != b"MDMP":
        print(f"{path}: not a minidump")
        continue
    print(f"== {path.split('/')[-1]} streams={nstreams}")
    mods = []
    exc = None
    threads = []
    for i in range(nstreams):
        stype, dsize, rva = DIR.unpack_from(data, dirstart + i * DIR.size)
        if stype == 4:
            count = struct.unpack_from("<I", data, rva)[0]
            off = rva + 4
            for _ in range(min(count, 80)):
                base, size, cksum, tstamp, nrva = MODENT.unpack_from(data, off)
                mods.append((base, size, modname(data, nrva)))
                off += 108
        elif stype == 6:
            tid, _ = EXC_STREAM.unpack_from(data, rva)
            code, flags, inner, addr, nparams, _ = EXC_REC.unpack_from(data, rva + 8)
            exc = (tid, code, addr)
        elif stype == 3:
            count = struct.unpack_from("<I", data, rva)[0]
            off = rva + 4
            for _ in range(min(count, 160)):
                tid, susp, pcls, prio, teb = struct.unpack_from("<I I I I Q", data, off)
                st_start, st_size, st_rva = struct.unpack_from("<Q I I", data, off + 24)
                tc_size, tc_rva = struct.unpack_from("<I I", data, off + 40)
                threads.append((tid, st_start, st_size, st_rva, tc_size, tc_rva))
                off += 48
    for base, size, name in mods:
        if name in WANT:
            print(f"   mod 0x{base:x} {name}")
    if not exc:
        print("   (no exception stream)")
        continue
    tid, code, addr = exc
    print(f"   thread={tid} code=0x{code:08x} ({EXC_NAMES.get(code, '?')}) addr=0x{addr:x}")

    def resolve(val):
        for base, size, name in mods:
            if base <= val < base + size:
                return f" <- {name}+0x{val - base:x}"
        return ""

    for ttid, st_start, st_size, st_rva, tc_size, tc_rva in threads:
        if ttid != tid:
            continue
        print(f"   stack 0x{st_start:x}+{st_size} ctxsize={tc_size}")
        try:
            rsp = struct.unpack_from("<Q", data, tc_rva + 0x98)[0]
            rip = struct.unpack_from("<Q", data, tc_rva + 0xF8)[0]
        except struct.error:
            print("   ctx_fail")
            break
        print(f"   rsp=0x{rsp:x} rip=0x{rip:x}{resolve(rip)}")
        ascii_hits = []
        for k in range(400):
            a = rsp + k * 8
            if a < st_start or a + 8 > st_start + st_size:
                break
            val = struct.unpack_from("<Q", data, st_rva + (a - st_start))[0]
            if k < 24:
                print(f"      +0x{k*8:03x}: 0x{val:016x}{resolve(val)}")
            for shift in (0, 8, 16, 24, 32, 40, 48, 56):
                chunk = bytes((val >> shift) & 0xFF for _ in range(8))
                try:
                    s = chunk.decode("ascii")
                except UnicodeDecodeError:
                    continue
                if all(32 <= ord(c) < 127 for c in s) and len(s.strip()) >= 6:
                    ascii_hits.append((a, s))
        for a, s in ascii_hits[:15]:
            print(f"      ascii@{a - rsp:+#x}: {s!r}")
        break
print("PARSE_DONE")
