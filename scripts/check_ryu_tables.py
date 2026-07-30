#!/usr/bin/env python3
# check_ryu_tables.py — verify the compressed Ryu pow5 constants in
# src/string/conversions/bits.hpp against exact first-principles values.
#
# Checks the CONSTANTS (anchors, base table, offset bitfields) by emulating the
# reconstruction exactly as pow5_compute / pow5_compute_inv perform it, for every
# index reachable from d2d: forward i in [0, 326), inverse q in [0, 291).
# It does not parse the C++ control flow — tests/rigor/rigor_format_ryu.cpp
# covers the code itself.
#
# Usage:  python3 scripts/check_ryu_tables.py [--emit]
#   --emit  also print corrected literal blocks for any mismatching table
#
# Exit status: 0 clean, 1 mismatches found.

import re
import sys
from pathlib import Path

MASK = (1 << 64) - 1
HDR = Path(__file__).resolve().parent.parent / "src/string/conversions/bits.hpp"


def pow5bits(e):
    return ((e * 1217359) >> 19) + 1


def exact_fwd(i):
    p = 5**i
    b = p.bit_length()
    return (p << (125 - b)) if b <= 125 else (p >> (b - 125))


def exact_inv(q):
    p = 5**q
    b = p.bit_length()
    return ((1 << (b - 1 + 125)) // p) + 1


def parse_array(src, name):
    m = re.search(rf"{name}\s*\[[0-9]*\]\s*(?:\[2\]\s*)?=\s*\{{(.*?)\}}\s*;", src, re.S)
    if not m:
        sys.exit(f"cannot find {name} in {HDR}")
    body = m.group(1)
    if "[2]" in src[m.start(): m.start() + len(name) + 24]:
        rows = re.findall(r"\{\s*([0-9]+)ull\s*,\s*([0-9]+)ull\s*\}", body)
        return [(int(a), int(b)) for a, b in rows]
    return [int(t, 0) for t in re.findall(r"0[xX][0-9a-fA-F]+|\d+", body.replace("ull", ""))]


def umul128(a, b):
    r = a * b
    return r & MASK, r >> 64


def shl128(lo, hi, d):
    if d == 0:
        return hi
    if d < 64:
        return ((hi << d) | (lo >> (64 - d))) & MASK
    return (lo << (d - 64)) & MASK


def reconstruct(i, anchors, offsets, table, inverse):
    if inverse:
        base = (i + 25) // 26
    else:
        base = i // 26
    base2 = base * 26
    mul = anchors[base]
    if i == base2:
        return mul
    m = table[base2 - i] if inverse else table[i - base2]
    lo0 = (mul[0] - 1) & MASK if inverse else mul[0]
    b0lo, b0hi = umul128(m, lo0)
    b2lo, b2hi = umul128(m, mul[1])
    mid = (b0hi + b2lo) & MASK
    high = (b2hi + (1 if mid < b0hi else 0)) & MASK
    delta = pow5bits(base2) - pow5bits(i) if inverse else pow5bits(i) - pow5bits(base2)
    corr = (offsets[i // 16] >> ((i % 16) << 1)) & 3
    r0 = (shl128(b0lo, mid, 64 - delta) + corr + (1 if inverse else 0)) & MASK
    return r0, shl128(mid, high, 64 - delta)


def main():
    emit = "--emit" in sys.argv
    src = HDR.read_text()
    table = parse_array(src, "__pow5_table")
    split2 = parse_array(src, "__pow5_split2")
    inv_split2 = parse_array(src, "__pow5_inv_split2")
    offsets = parse_array(src, "__pow5_offsets")
    inv_offsets = parse_array(src, "__pow5_inv_offsets")

    bad = 0
    if table != [5**k for k in range(26)]:
        bad += 1
        print("FAIL __pow5_table")
    for label, n, anchors, offs, exact, inverse in (
        ("fwd", 326, split2, offsets, exact_fwd, False),
        ("inv", 291, inv_split2, inv_offsets, exact_inv, True),
    ):
        miss = []
        for i in range(n):
            ex = exact(i)
            if reconstruct(i, anchors, offs, table, inverse) != (ex & MASK, ex >> 64):
                miss.append(i)
        if miss:
            bad += 1
            print(f"FAIL {label}: {len(miss)} of {n} indices wrong: {miss[:12]}{'...' if len(miss) > 12 else ''}")
        else:
            print(f"ok   {label}: all {n} reconstructed entries exact")

    if bad and emit:
        print("\ncorrected anchors (lo, hi):")
        for b in range(13):
            ex = exact_fwd(26 * b)
            print(f"  split2[{b}] = {{ {ex & MASK}ull, {ex >> 64}ull }}")
        words = [0] * 21
        for i in range(326):
            base2 = (i // 26) * 26
            if i == base2:
                continue
            ex = exact_fwd(i)
            anch = (exact_fwd(base2) & MASK, exact_fwd(base2) >> 64)
            b0lo, b0hi = umul128(table[i - base2], anch[0])
            b2lo, b2hi = umul128(table[i - base2], anch[1])
            mid = (b0hi + b2lo) & MASK
            delta = pow5bits(i) - pow5bits(base2)
            need = ((ex & MASK) - shl128(b0lo, mid, 64 - delta)) & MASK
            assert need <= 3, (i, need)
            words[i // 16] |= need << ((i % 16) << 1)
        print("corrected __pow5_offsets:")
        print("  { " + ", ".join(f"0x{w:08X}" for w in words) + " }")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
