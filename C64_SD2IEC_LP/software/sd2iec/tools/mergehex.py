#!/usr/bin/env python3
# ===================================================================================
# Merge Intel HEX files (firmware + bootloader) into one file for flashing
# ===================================================================================
# Usage: mergehex.py <output.hex> <input1> <input2> ...
#
# Inputs are Intel HEX files or binary files (*.bin, loaded at address 0; bytes
# 0xFF are left out, as they are the erased state of the flash). Overlapping data
# is not allowed. One file avoids problems of some avrdude versions with several
# flash writes in one session.
#
# 2026 by Stefan Wagner:   https://github.com/wagiminator
import sys

def read_hex(name):
    mem, base = {}, 0
    for line in open(name):
        line = line.strip()
        if not line.startswith(':'): continue
        rec = bytes.fromhex(line[1:])
        if sum(rec) & 0xFF: sys.exit('%s: checksum error' % name)
        n, addr, typ = rec[0], rec[1] << 8 | rec[2], rec[3]
        if typ == 0:
            for i in range(n):
                a = base + addr + i
                if a in mem: sys.exit('%s: overlapping data at 0x%05X' % (name, a))
                mem[a] = rec[4 + i]
        elif typ == 1: break
        elif typ == 2: base = (rec[4] << 8 | rec[5]) << 4
        elif typ == 4: base = (rec[4] << 8 | rec[5]) << 16
    return mem

def read_bin(name):
    return {a: v for a, v in enumerate(open(name, 'rb').read()) if v != 0xFF}

def write_hex(name, mem):
    out, ext = [], -1
    def rec(addr, typ, data):
        r = bytes([len(data), addr >> 8 & 0xFF, addr & 0xFF, typ]) + bytes(data)
        out.append(':' + (r + bytes([-sum(r) & 0xFF])).hex().upper())
    addrs = sorted(mem)
    i = 0
    while i < len(addrs):
        start = addrs[i]
        if start >> 16 != ext:
            ext = start >> 16
            rec(0, 4, [ext >> 8, ext & 0xFF])
        data = [mem[start]]
        while (i + len(data) < len(addrs) and len(data) < 16 and
               addrs[i + len(data)] == start + len(data) and (start + len(data)) & 0xFFFF):
            data.append(mem[start + len(data)])
        rec(start & 0xFFFF, 0, data)
        i += len(data)
    rec(0, 1, [])
    open(name, 'w').write('\n'.join(out) + '\n')

merged = {}
for name in sys.argv[2:]:
    data = read_bin(name) if name.lower().endswith('.bin') else read_hex(name)
    for a, v in data.items():
        if a in merged: sys.exit('overlapping data at 0x%05X' % a)
        merged[a] = v
write_hex(sys.argv[1], merged)
