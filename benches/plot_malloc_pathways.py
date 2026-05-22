#!/usr/bin/env python3
"""
Turn malloc_pathways bench output (single-threaded + multi-threaded) into PNG charts.

Capture the bench text first (ANSI bold on abc rows is stripped automatically):
    ./bin/malloc_pathways_bench    > st.txt
    ./bin/malloc_pathways_mt_bench > mt.txt

Then render:
    python3 benches/plot_malloc_pathways.py st.txt mt.txt --outdir benches/charts

Files are auto-detected as ST or MT from their banner.  Charts produced:
    ST  per (pathway, phase):  *_cycop.png   (grouped bars: cyc/op by count, per tier)
                               *_latency.png (lines: p10..max latency, per tier)
    MT  per pathway:           *_scaling.png (lines: Mops/s vs threads, per tier)
                               *_cycop.png   (grouped bars: cyc/op vs threads, per tier)
                               *_latency.png (lines: p10..max latency at max threads, per tier)
"""
import argparse
import math
import os
import re
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
import numpy as np

ANSI = re.compile(r"\x1b\[[0-9;]*m")
KNOWN_IMPLS = {"abc", "sys", "mi", "je", "abc-lnd", "abc-alc"}
IMPL_ORDER = ["abc", "abc-lnd", "abc-alc", "sys", "mi", "je"]
IMPL_LABEL = {"abc": "abcmalloc", "abc-lnd": "abc launder", "abc-alc": "abc alloc",
              "sys": "glibc", "mi": "mimalloc", "je": "jemalloc"}
IMPL_COLOR = {"abc": "#d62728", "abc-lnd": "#ff7f0e", "abc-alc": "#8c564b",
              "sys": "#7f7f7f", "mi": "#1f77b4", "je": "#2ca02c"}
ST_CATS = ["tiny", "small", "sub-page", "medium", "large", "huge", "mixed"]
MT_CATS = ["tiny", "small", "medium", "large"]
PCTS = ["p10", "p50", "p90", "p99", "p999", "max"]
PCT_LABELS = ["p10", "p50", "p90", "p99", "p99.9", "max"]


def parse(path):
    bench = section = mode = None
    out = []
    with open(path, encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = ANSI.sub("", raw.rstrip("\n"))
            s = line.strip()
            if not s:
                continue
            if "single-threaded" in s:
                bench = "st"; continue
            if "multi-threaded" in s:
                bench = "mt"; continue
            m = re.match(r"\[([a-f])\]\s+([A-Za-z]+)", s)
            if m:
                section = m.group(2).lower(); mode = None; continue
            if s.startswith("throughput"):
                mode = "tput"; continue
            if s.startswith("per-op latency"):
                mode = "lat"; continue
            if s.startswith("impl") or s.startswith("---"):
                continue
            tok = s.split()
            if not tok or tok[0] not in KNOWN_IMPLS or bench is None or section is None or mode is None:
                continue
            try:
                rec = _row(bench, section, mode, tok)
            except (ValueError, IndexError):
                continue
            if rec:
                out.append(rec)
    return out


def _row(bench, section, mode, t):
    base = dict(bench=bench, section=section, mode=mode, impl=t[0], cat=t[1])
    if bench == "st" and mode == "tput":
        # impl cat phase count ops cycop nsop mops ipc bmiss [ratio]
        base.update(phase=t[2], count=int(t[3]), ops=int(t[4]), cycop=float(t[5]),
                    nsop=float(t[6]), mops=float(t[7]), ipc=float(t[8]), bmiss=float(t[9]),
                    ratio=float(t[10]) if len(t) >= 11 else None)
        return base
    if bench == "st" and mode == "lat":
        base.update(phase=t[2], count=int(t[3]),
                    lat={p: float(t[4 + i]) for i, p in enumerate(PCTS)})
        return base
    if bench == "mt" and mode == "tput":
        if len(t) >= 10:
            base.update(thr=int(t[2]), total=int(t[3]), mops=float(t[4]), scaling=float(t[5]),
                        cycop=float(t[6]), ratio=float(t[7]), ipc=float(t[8]), bmiss=float(t[9]))
        else:
            base.update(thr=int(t[2]), total=int(t[3]), mops=float(t[4]), scaling=float(t[5]),
                        cycop=float(t[6]), ratio=None, ipc=float(t[7]), bmiss=float(t[8]))
        return base
    if bench == "mt" and mode == "lat":
        base.update(thr=int(t[2]), total=int(t[3]),
                    lat={p: float(t[4 + i]) for i, p in enumerate(PCTS)})
        return base
    return None


def human(n):
    if n >= 1_000_000 and n % 1_000_000 == 0:
        return f"{n // 1_000_000}M"
    if n >= 1000 and n % 1000 == 0:
        return f"{n // 1000}k"
    return str(n)


def present(recs, key, order):
    have = {r[key] for r in recs}
    return [v for v in order if v in have]


def grid(n, ncols):
    ncols = max(1, min(ncols, n))
    nrows = max(1, math.ceil(n / ncols))
    fig, axes = plt.subplots(nrows, ncols, figsize=(5.4 * ncols, 3.7 * nrows), squeeze=False)
    return fig, [axes[r][c] for r in range(nrows) for c in range(ncols)]


def finish(fig, impls, title, path):
    handles = [Patch(facecolor=IMPL_COLOR[i], label=IMPL_LABEL[i]) for i in impls]
    fig.suptitle(title, fontsize=13)
    fig.tight_layout(rect=(0, 0.06, 1, 0.95))      # leave room: title on top, legend at bottom
    fig.legend(handles=handles, loc="lower center", ncol=len(impls), frameon=False)
    fig.savefig(path, dpi=120)
    plt.close(fig)
    print("wrote", path)


def bars(ax, crs, impls, xkey, xvals, ykey, xlabel, ylabel):
    x = np.arange(len(xvals))
    w = 0.8 / max(1, len(impls))
    for k, impl in enumerate(impls):
        ys = []
        for xv in xvals:
            v = [r[ykey] for r in crs if r["impl"] == impl and r[xkey] == xv]
            ys.append(v[0] if v else 0.0)
        ax.bar(x + (k - (len(impls) - 1) / 2) * w, ys, w, color=IMPL_COLOR[impl])
    ax.set_xticks(x)
    ax.set_xticklabels([human(v) if isinstance(v, int) else str(v) for v in xvals])
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.grid(axis="y", alpha=0.3)


def pctl_lines(ax, crs, impls, title):
    x = np.arange(len(PCTS))
    for impl in impls:
        row = [r for r in crs if r["impl"] == impl]
        if not row:
            continue
        ax.plot(x, [row[0]["lat"][p] for p in PCTS], marker="o", color=IMPL_COLOR[impl])
    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(PCT_LABELS, rotation=45)
    ax.set_ylabel("ns/op (log)")
    ax.set_title(title)
    ax.grid(alpha=0.3, which="both")


def st_throughput(recs, outdir):
    g = defaultdict(list)
    for r in recs:
        if r["mode"] == "tput":
            g[(r["section"], r["phase"])].append(r)
    for (sec, phase), rs in g.items():
        cats = present(rs, "cat", ST_CATS)
        impls = present(rs, "impl", IMPL_ORDER)
        fig, axes = grid(len(cats), 3)
        for ax, cat in zip(axes, cats):
            crs = [r for r in rs if r["cat"] == cat]
            counts = sorted({r["count"] for r in crs})
            bars(ax, crs, impls, "count", counts, "cycop", "count", "cyc/op")
            ax.set_title(cat)
        for ax in axes[len(cats):]:
            ax.axis("off")
        finish(fig, impls, f"ST  {sec} [{phase}]  —  cyc/op by count, per size tier (lower = better)",
               os.path.join(outdir, f"st_{sec}_{phase}_cycop.png"))


def st_latency(recs, outdir):
    g = defaultdict(list)
    for r in recs:
        if r["mode"] == "lat":
            g[(r["section"], r["phase"])].append(r)
    for (sec, phase), rs in g.items():
        cats = present(rs, "cat", ST_CATS)
        impls = present(rs, "impl", IMPL_ORDER)
        fig, axes = grid(len(cats), 3)
        for ax, cat in zip(axes, cats):
            crs = [r for r in rs if r["cat"] == cat]
            maxc = max(r["count"] for r in crs)
            pctl_lines(ax, [r for r in crs if r["count"] == maxc], impls, f"{cat} @{human(maxc)}")
        for ax in axes[len(cats):]:
            ax.axis("off")
        finish(fig, impls, f"ST  {sec} [{phase}]  —  per-op latency percentiles (ns, log scale)",
               os.path.join(outdir, f"st_{sec}_{phase}_latency.png"))


def mt_scaling(recs, outdir):
    g = defaultdict(list)
    for r in recs:
        if r["mode"] == "tput":
            g[r["section"]].append(r)
    for sec, rs in g.items():
        cats = present(rs, "cat", MT_CATS)
        impls = present(rs, "impl", IMPL_ORDER)
        fig, axes = grid(len(cats), 2)
        for ax, cat in zip(axes, cats):
            crs = [r for r in rs if r["cat"] == cat]
            threads = sorted({r["thr"] for r in crs})
            for impl in impls:
                xs, ys = [], []
                for th in threads:
                    v = [r["mops"] for r in crs if r["impl"] == impl and r["thr"] == th]
                    if v:
                        xs.append(th); ys.append(v[0])
                ax.plot(xs, ys, marker="o", color=IMPL_COLOR[impl])
            ax.set_title(cat); ax.set_xlabel("threads"); ax.set_ylabel("Mops/s")
            ax.set_xticks(threads); ax.grid(alpha=0.3)
        for ax in axes[len(cats):]:
            ax.axis("off")
        finish(fig, impls, f"MT  {sec}  —  throughput scaling: Mops/s vs threads (higher = better)",
               os.path.join(outdir, f"mt_{sec}_scaling.png"))


def mt_cycop(recs, outdir):
    g = defaultdict(list)
    for r in recs:
        if r["mode"] == "tput":
            g[r["section"]].append(r)
    for sec, rs in g.items():
        cats = present(rs, "cat", MT_CATS)
        impls = present(rs, "impl", IMPL_ORDER)
        fig, axes = grid(len(cats), 2)
        for ax, cat in zip(axes, cats):
            crs = [r for r in rs if r["cat"] == cat]
            threads = sorted({r["thr"] for r in crs})
            bars(ax, crs, impls, "thr", threads, "cycop", "threads", "cyc/op")
            ax.set_title(cat)
        for ax in axes[len(cats):]:
            ax.axis("off")
        finish(fig, impls, f"MT  {sec}  —  cyc/op vs threads, per size tier (lower = better)",
               os.path.join(outdir, f"mt_{sec}_cycop.png"))


def mt_latency(recs, outdir):
    g = defaultdict(list)
    for r in recs:
        if r["mode"] == "lat":
            g[r["section"]].append(r)
    for sec, rs in g.items():
        cats = present(rs, "cat", MT_CATS)
        impls = present(rs, "impl", IMPL_ORDER)
        fig, axes = grid(len(cats), 2)
        for ax, cat in zip(axes, cats):
            crs = [r for r in rs if r["cat"] == cat]
            maxt = max(r["thr"] for r in crs)
            pctl_lines(ax, [r for r in crs if r["thr"] == maxt], impls, f"{cat} @{maxt}T")
        for ax in axes[len(cats):]:
            ax.axis("off")
        finish(fig, impls, f"MT  {sec}  —  per-op latency percentiles at max threads (ns, log)",
               os.path.join(outdir, f"mt_{sec}_latency.png"))


def main():
    ap = argparse.ArgumentParser(description="malloc_pathways bench output -> PNG charts")
    ap.add_argument("inputs", nargs="+", help="captured bench text file(s) (ST and/or MT)")
    ap.add_argument("--outdir", default="benches/charts", help="output directory for PNGs")
    args = ap.parse_args()
    os.makedirs(args.outdir, exist_ok=True)

    recs = []
    for p in args.inputs:
        recs += parse(p)
    st = [r for r in recs if r["bench"] == "st"]
    mt = [r for r in recs if r["bench"] == "mt"]
    print(f"parsed {len(st)} ST rows, {len(mt)} MT rows")

    if st:
        st_throughput(st, args.outdir)
        st_latency(st, args.outdir)
    if mt:
        mt_scaling(mt, args.outdir)
        mt_cycop(mt, args.outdir)
        mt_latency(mt, args.outdir)


if __name__ == "__main__":
    main()
