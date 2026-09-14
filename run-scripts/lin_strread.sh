#!/bin/bash
# Read candidate strings from the fatal function's references.
set -u
F=/home/kubuntu/adobe-wine-lab/webview2/153.0.4234.32/msedge.dll
python3 - "$F" <<'EOF'
import struct, sys
d = open(sys.argv[1], "rb").read()
pe = struct.unpack_from("<I", d, 0x3C)[0]
nsec = struct.unpack_from("<H", d, pe + 6)[0]
def rva_to_off(rva):
    off = pe + 24 + struct.unpack_from("<H", d, pe + 20)[0]
    for _ in range(nsec):
        vsz, vaddr, rawsz, raw = struct.unpack_from("<IIII", d, off + 8)
        if vaddr <= rva < vaddr + max(vsz, rawsz):
            return raw + (rva - vaddr)
        off += 40
    return None
for vma in (0x192581460, 0x193BFC660, 0x1910268B0, 0x193D42CC8, 0x193AB8040):
    rva = vma - 0x180000000
    off = rva_to_off(rva)
    if off is None:
        print(f"{vma:#x}: unmapped")
        continue
    raw = d[off:off + 160]
    asc = bytes(b if 32 <= b < 127 else 46 for b in raw).decode()
    print(f"{vma:#x}: A={asc[:100]}")
    try:
        u16 = raw.decode("utf-16-le")
        u16c = "".join(c if 32 <= ord(c) < 127 or c in "\r\n\t" else "" for c in u16[:80])
        print(f"{vma:#x}: W={u16c[:100]}")
    except Exception:
        pass
EOF
echo STRREAD_DONE
