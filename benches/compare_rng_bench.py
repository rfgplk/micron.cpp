#!/usr/bin/env python3
"""Compare two rng_bench CSV captures with one noise-resistant aggregate gate."""

from __future__ import annotations

import argparse
import csv
import math
import sys
from pathlib import Path


def load(path: Path) -> dict[tuple[str, str], tuple[float, float]]:
    rows: dict[tuple[str, str], tuple[float, float]] = {}
    with path.open(newline="") as stream:
        lines = (line for line in stream if line.strip() and not line.startswith("#"))
        for row in csv.DictReader(lines):
            key = (row["group"], row["name"])
            if key[0] == "hardware":
                continue
            if key == ("sampler", "normal_polar"):
                key = ("sampler", "normal_default")
            rows[key] = (float(row["cycles_x1000"]), float(row["ipc_x1000"]))
    return rows


def geometric_mean(values: list[float]) -> float:
    return math.exp(sum(math.log(value) for value in values) / len(values))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("baseline", type=Path)
    parser.add_argument("current", type=Path)
    parser.add_argument("--max-regression", type=float, default=1.05)
    parser.add_argument("--top", type=int, default=10)
    args = parser.parse_args()

    baseline = load(args.baseline)
    current = load(args.current)
    missing = sorted(baseline.keys() - current.keys())
    if missing:
        print("rng gate: current capture is missing baseline rows:", file=sys.stderr)
        for group, name in missing:
            print(f"  {group}/{name}", file=sys.stderr)
        return 2
    keys = sorted(baseline.keys() & current.keys())
    if not keys:
        print("rng gate: no matching benchmark rows", file=sys.stderr)
        return 2

    cycle_ratios = [current[key][0] / baseline[key][0] for key in keys]
    ipc_ratios = [current[key][1] / baseline[key][1] for key in keys if baseline[key][1] > 0]
    aggregate = geometric_mean(cycle_ratios)
    speedup = 1.0 / aggregate
    ipc_gain = geometric_mean(ipc_ratios)

    print(f"matched rows: {len(keys)}")
    print(f"aggregate speedup: {speedup:.4f}x")
    print(f"aggregate IPC change: {ipc_gain:.4f}x")
    print("largest cycle regressions:")
    for key, ratio in sorted(zip(keys, cycle_ratios), key=lambda item: item[1], reverse=True)[: args.top]:
        print(f"  {key[0]}/{key[1]}: {ratio:.4f}x baseline cycles")

    if aggregate > args.max_regression:
        print(f"rng gate failed: aggregate cycle ratio {aggregate:.4f} > {args.max_regression:.4f}", file=sys.stderr)
        return 1
    print(f"rng gate passed: aggregate cycle ratio {aggregate:.4f} <= {args.max_regression:.4f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
