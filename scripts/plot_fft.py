#!/usr/bin/env python3
"""Plot pair_fft.py results (optional analysis dependency: matplotlib)."""
import argparse
import csv
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("summary", type=Path)
    parser.add_argument("output", type=Path, help="output filename prefix")
    args = parser.parse_args()
    with args.summary.open() as stream:
        rows = list(csv.DictReader(stream))
    fig, axes = plt.subplots(1, 2, figsize=(10, 4.5), constrained_layout=True)
    for axis, precision in zip(axes, ("f32", "f64")):
        data = sorted((r for r in rows if r["operation"] == "c2c" and r["precision"] == precision
                       and int(r["n"]) & (int(r["n"]) - 1) == 0), key=lambda r: int(r["n"]))
        for field, label, color in (("before_ns", "Original micron", "#667085"),
                                    ("after_ns", "Tuned micron", "#1468b3"),
                                    ("eigen_ns", "Eigen KissFFT", "#c66c20")):
            available = [r for r in data if r[field]]
            axis.plot([int(r["n"]) for r in available], [float(r[field]) / 1000 for r in available],
                      marker="o", markersize=4, color=color, label=label)
        axis.set_xscale("log", base=2)
        axis.set_yscale("log")
        axis.set_xlabel("Complex samples (N)")
        axis.set_ylabel("Microseconds per forward transform")
        axis.set_title(precision)
        axis.grid(True, which="major", alpha=0.2)
        axis.spines[["top", "right"]].set_visible(False)
    axes[0].legend(frameon=False, loc="upper left")
    fig.suptitle("Prepared C2C • Haswell / GCC 16 / AVX2\nAlternating runs; frequency and SMT sibling uncontrolled", fontsize=11)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(str(args.output) + ".svg")
    fig.savefig(str(args.output) + ".png", dpi=160)


if __name__ == "__main__":
    main()
