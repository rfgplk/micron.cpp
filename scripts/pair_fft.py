#!/usr/bin/env python3
"""Alternate original/tuned FFT runs to reduce time-order bias on a busy host."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before", type=Path)
    parser.add_argument("after", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--cpu", type=int, default=2)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--eigen", type=Path)
    args = parser.parse_args()
    if args.rounds < 1:
        parser.error("--rounds must be positive")
    args.output.mkdir(parents=True, exist_ok=True)
    binaries = {"before": args.before.resolve(), "after": args.after.resolve()}
    if args.eigen:
        binaries["eigen"] = args.eigen.resolve()
    metadata = {"binaries": {k: {"path": str(v), "sha256": hashlib.sha256(v.read_bytes()).hexdigest()}
                              for k, v in binaries.items()}, "cpu": args.cpu, "rounds": args.rounds,
                "started": time.strftime("%Y-%m-%dT%H:%M:%S%z")}
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    cases = []
    for precision in ("f32", "f64"):
        cases += [("c2c", precision, n) for n in (16, 64, 256, 1000, 1009, 1024, 4096, 16384, 262144, 1048576)]
        cases += [(mode, precision, 1024) for mode in ("r2c", "c2r", "strided", "split", "multiply", "plan")]
        cases += [("nd", precision, 128)]
    rows, commands = [], []
    for repeat in range(args.rounds):
        for mode, precision, n in cases:
            order = ["before", "after"] if repeat % 2 == 0 else ["after", "before"]
            if args.eigen and mode == "c2c":
                order.append("eigen")
            for variant in order:
                command = ["taskset", "-c", str(args.cpu), str(binaries[variant])]
                command += [precision, str(n)] if variant == "eigen" else [mode, precision, str(n), "0", "7", "0", "1"]
                result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=180)
                commands.append({"argv": command, "returncode": result.returncode})
                (args.output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
                result.check_returncode()
                parsed = list(csv.DictReader(result.stdout.splitlines()))
                if len(parsed) != 1:
                    raise RuntimeError(f"Expected one row: {result.stdout}")
                row = parsed[0]
                rows.append({"round": repeat, "variant": variant, "operation": mode, "precision": precision, "n": n,
                             **{key: row[key] for key in ("iterations", "median_ns", "min_ns", "p25_ns", "p75_ns", "max_ns")}})
    with (args.output / "samples.csv").open("w") as stream:
        writer = csv.DictWriter(stream, rows[0])
        writer.writeheader()
        writer.writerows(rows)
    summaries = []
    for mode, precision, n in cases:
        subset = [r for r in rows if (r["operation"], r["precision"], r["n"]) == (mode, precision, n)]
        times = {v: [float(r["median_ns"]) for r in subset if r["variant"] == v] for v in binaries}
        ratios = [b / a for b, a in zip(times["before"], times["after"])]
        summaries.append({"operation": mode, "precision": precision, "n": n,
                          "before_ns": statistics.median(times["before"]), "after_ns": statistics.median(times["after"]),
                          "paired_speedup_median": statistics.median(ratios),
                          "paired_speedup_min": min(ratios), "paired_speedup_max": max(ratios),
                          "eigen_ns": statistics.median(times["eigen"]) if times.get("eigen") else ""})
    with (args.output / "summary.csv").open("w") as stream:
        writer = csv.DictWriter(stream, summaries[0])
        writer.writeheader()
        writer.writerows(summaries)


if __name__ == "__main__":
    main()
