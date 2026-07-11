#!/usr/bin/env python3
"""
Compare two memory_detail_bench / memory_random_bench CSV captures.

    ./bin/memory_detail_bench --csv > benches/results/old.csv
    # ... change the implementation ...
    ./bin/memory_detail_bench --csv > benches/results/new.csv
    python3 benches/compare_mem_bench.py benches/results/old.csv benches/results/new.csv

Reports, glibc-commit style (New/Old, < 1.00 is an improvement):
  - top regressions / improvements for --impl (default: micron)
  - geometric mean New/Old per op x size-class x mode
  - overall geomean per op and across the suite
  - a libc-rows control check: libc did not change, so its geomean must
    stay ~1.00; a deviation > 5% means the environment shifted between runs

Optional: --plot benches/charts  (cyc/B vs size, log-x, old vs new vs libc)
"""
import argparse
import math
import os
from collections import defaultdict

KEY = ("op", "impl", "size", "dst_align", "src_align", "mode", "delta")

SIZE_CLASSES = [
    (0, 32, "[0,32]"),
    (33, 256, "(32,256]"),
    (257, 2048, "(256,2048]"),
    (2049, 65536, "(2048,64Ki]"),
    (65537, None, "(64Ki,L3]"),  # upper bound patched from l3_bytes
    (None, None, "(L3,inf)"),
]


def parse(path):
    rows = {}
    meta = {}
    with open(path) as f:
        for ln in f:
            ln = ln.strip()
            if not ln:
                continue
            if ln.startswith("#"):
                for tok in ln[1:].split():
                    if "=" in tok:
                        k, v = tok.split("=", 1)
                        meta[k] = v
                continue
            parts = ln.split(",")
            if parts[0] == "op":  # header
                continue
            if len(parts) != 9:
                continue
            op, impl, size, d_al, s_al, mode, delta, cpb, gib = parts
            key = (op, impl, int(size), int(d_al), int(s_al), mode, int(delta))
            rows[key] = (float(cpb), float(gib))
    return rows, meta


def size_class(n, l3):
    for lo, hi, name in SIZE_CLASSES:
        if lo is None:  # (L3,inf)
            if n > l3:
                return name
            continue
        h = hi if hi is not None else l3
        if lo <= n <= h:
            return name
    return "(L3,inf)"


def ratio(old, new):
    """New/Old on cyc_per_byte; fall back to inverse gib ratio when cyc is 0."""
    ocpb, ogib = old
    ncpb, ngib = new
    if ocpb > 0 and ncpb > 0:
        return ncpb / ocpb
    if ogib > 0 and ngib > 0:
        return ogib / ngib
    return None


def geomean(vals):
    vals = [v for v in vals if v and v > 0]
    if not vals:
        return None
    return math.exp(sum(math.log(v) for v in vals) / len(vals))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("old_csv")
    ap.add_argument("new_csv")
    ap.add_argument("--impl", default="micron")
    ap.add_argument("--op", default=None, help="restrict to one op")
    ap.add_argument("--top", type=int, default=15)
    ap.add_argument("--plot", default=None, help="output dir for charts")
    args = ap.parse_args()

    old, ometa = parse(args.old_csv)
    new, nmeta = parse(args.new_csv)
    l3 = int(nmeta.get("l3_bytes", ometa.get("l3_bytes", 32 << 20)))

    joined = []
    for k, ov in old.items():
        nv = new.get(k)
        if nv is None:
            continue
        r = ratio(ov, nv)
        if r is None:
            continue
        joined.append((k, ov, nv, r))
    if not joined:
        print("no joinable rows — did both runs use --csv and the same matrix?")
        return 1

    # control: libc rows should not move
    libc_r = [r for (k, _, _, r) in joined if k[1] == "libc"]
    g_libc = geomean(libc_r)
    if g_libc is not None:
        flag = "" if abs(g_libc - 1.0) <= 0.05 else "  *** WARNING: environment shifted, re-run! ***"
        print(f"control geomean New/Old (libc, n={len(libc_r)}): {g_libc:.3f}{flag}")

    sel = [j for j in joined if j[0][1] == args.impl and (args.op is None or j[0][0] == args.op)]
    if not sel:
        print(f"no rows for impl={args.impl}")
        return 1

    sel.sort(key=lambda j: j[3])
    print(f"\ntop {args.top} improvements (impl={args.impl}):")
    print(f"{'ratio':>7}  {'op':<12}{'size':>10}{'d/s_al':>9}{'mode':>6}{'delta':>7}   old->new cyc/B")
    for k, ov, nv, r in sel[: args.top]:
        print(f"{r:7.3f}  {k[0]:<12}{k[2]:>10}{str(k[3]) + '/' + str(k[4]):>9}{k[5]:>6}{k[6]:>7}   {ov[0]:.4f} -> {nv[0]:.4f}")
    print(f"\ntop {args.top} regressions (impl={args.impl}):")
    for k, ov, nv, r in sel[-args.top:][::-1]:
        if r <= 1.0:
            break
        print(f"{r:7.3f}  {k[0]:<12}{k[2]:>10}{str(k[3]) + '/' + str(k[4]):>9}{k[5]:>6}{k[6]:>7}   {ov[0]:.4f} -> {nv[0]:.4f}")

    # geomean per op x size-class x mode
    buckets = defaultdict(list)
    per_op = defaultdict(list)
    for k, ov, nv, r in sel:
        cls = size_class(k[2], l3)
        buckets[(k[0], cls, k[5])].append(r)
        per_op[k[0]].append(r)

    print(f"\ngeomean New/Old per op x size-class x mode (impl={args.impl}):")
    classes = [c[2] for c in SIZE_CLASSES]
    ops = sorted(per_op)
    for mode in ("hot", "cold", "rand"):
        rows = [(op, [buckets.get((op, c, mode)) for c in classes]) for op in ops]
        if not any(any(v) for _, v in rows):
            continue
        print(f"  mode={mode}")
        print("    " + f"{'op':<12}" + "".join(f"{c:>13}" for c in classes))
        for op, cells in rows:
            line = f"    {op:<12}"
            for vals in cells:
                g = geomean(vals) if vals else None
                line += f"{g:{13}.3f}" if g else f"{'-':>13}"
            print(line)

    print(f"\ngeomean New/Old per op (impl={args.impl}):")
    allr = []
    for op in ops:
        g = geomean(per_op[op])
        allr.extend(per_op[op])
        print(f"  {op:<14} {g:.3f}   (n={len(per_op[op])})")
    print(f"  {'ALL':<14} {geomean(allr):.3f}   (n={len(allr)})")

    if args.plot:
        plot(args.plot, old, new, args.impl)
    return 0


def plot(outdir, old, new, impl):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    os.makedirs(outdir, exist_ok=True)
    for op in ("memcpy", "memset0", "memmove"):
        series = defaultdict(dict)  # label -> {size: cpb}
        for src, tag in ((old, "old"), (new, "new")):
            for k, (cpb, _) in src.items():
                if k[0] != op or k[3] != 0 or k[4] != 0 or k[5] != "hot" or k[6] != 0:
                    continue
                if k[1] == impl:
                    series[f"{impl}-{tag}"][k[2]] = cpb
                elif k[1] == "libc" and tag == "new":
                    series["libc"][k[2]] = cpb
        if not series:
            continue
        fig, ax = plt.subplots(figsize=(9, 5))
        for label, pts in sorted(series.items()):
            xs = sorted(x for x in pts if x > 0)
            ax.plot(xs, [pts[x] for x in xs], marker=".", label=label)
        ax.set_xscale("log", base=2)
        ax.set_xlabel("size (bytes)")
        ax.set_ylabel("cyc/byte (lower is better)")
        ax.set_title(f"{op}: aligned hot path")
        ax.legend()
        ax.grid(True, alpha=0.3)
        fig.tight_layout()
        fig.savefig(os.path.join(outdir, f"mem_{op}_cycb.png"), dpi=120)
        plt.close(fig)
    print(f"charts written to {outdir}/")


if __name__ == "__main__":
    raise SystemExit(main())
