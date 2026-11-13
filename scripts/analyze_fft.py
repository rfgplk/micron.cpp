#!/usr/bin/env python3
"""Compare bench_fft.py runs and summarize Haswell counters without hiding missing events."""
import argparse
import csv
import json
from pathlib import Path


def write_csv(path, rows):
    if rows:
        with path.open("w") as stream:
            writer = csv.DictWriter(stream, rows[0])
            writer.writeheader()
            writer.writerows(rows)


def timings(path):
    with path.open() as stream:
        return {(r["operation"], r["precision"], r["n"], r["offset"], r["workers"]): r
                for r in csv.DictReader(stream)}


def counters(path):
    result = {}
    for row in csv.reader(path.read_text().splitlines()):
        if not row or row[0].startswith("#"):
            continue
        # perf stat -x, -r: count,unit,event,stddev,time_enabled,running%,...
        if len(row) < 6 or row[0].startswith("<"):
            raise ValueError(f"Unusable event in {path}: {row}")
        result[row[2].removesuffix(":u")] = {
            "count": float(row[0]), "stddev_pct": float(row[3].rstrip("%")),
            "running_pct": float(row[5]),
        }
        if result[row[2].removesuffix(":u")]["running_pct"] < 95:
            raise ValueError(f"Counter ran less than 95% of enabled time: {path}: {row}")
    if not result:
        raise ValueError(f"Empty perf file: {path}")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before", type=Path)
    parser.add_argument("after", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    before, after = (timings(d / "timings.csv") for d in (args.before, args.after))
    comparisons = []
    for key, a in after.items():
        if key not in before:
            continue
        b = before[key]
        comparisons.append(dict(zip(("operation", "precision", "n", "offset", "workers"), key)) | {
            "before_ns": b["median_ns"], "after_ns": a["median_ns"],
            "speedup": float(b["median_ns"]) / float(a["median_ns"]),
            "before_iqr_over_median": (float(b["p75_ns"]) - float(b["p25_ns"])) / float(b["median_ns"]),
            "after_iqr_over_median": (float(a["p75_ns"]) - float(a["p25_ns"])) / float(a["median_ns"]),
        })
    write_csv(args.output / "comparison.csv", comparisons)
    metrics, raw, errors = [], [], []
    for variant, directory in (("before", args.before), ("after", args.after)):
        for mode, precision, n, iterations in (("c2c", "f64", 1024, 100000), ("c2c", "f32", 4096, 40000),
                                               ("multiply", "f64", 4096, 100000), ("strided", "f64", 1024, 30000)):
            groups = {}
            for path in sorted(directory.glob(f"perf-{mode}-{precision}-{n}-*.csv")):
                try:
                    groups[path.stem.rsplit("-", 1)[1]] = counters(path)
                except ValueError as e:
                    errors.append(str(e))
            if not groups:
                continue
            identity = {"variant": variant, "operation": mode, "precision": precision, "n": n}
            for group, events in groups.items():
                cycles = events["cycles"]["count"]
                for name, event in events.items():
                    raw.append(identity | {"group": group, "event": name, **event,
                                          "per_requested_call": event["count"] / iterations,
                                          "per_cycle": event["count"] / cycles})

            def rate(group, event):
                return groups[group][event]["count"] / groups[group]["cycles"]["count"]

            if "slots" in groups and "retire" in groups:
                frontend = rate("slots", "idq_uops_not_delivered.core") / 4
                retiring = rate("slots", "uops_retired.retire_slots") / 4
                speculation = (rate("slots", "uops_issued.any") - rate("slots", "uops_retired.retire_slots")) / 4
                speculation += rate("retire", "int_misc.recovery_cycles")
                # Recovery is from a separate run; the SMT sibling is uncontrolled. These are estimates.
                # Preserve negative residuals instead of clamping away evidence of measurement error.
                metrics.append(identity | {
                    "cycles_per_requested_call": groups["slots"]["cycles"]["count"] / iterations,
                    "instructions_per_requested_call": groups["slots"]["instructions"]["count"] / iterations,
                    "ipc": rate("slots", "instructions"),
                    "frontend_slot_estimate_pct": frontend * 100,
                    "retiring_slot_estimate_pct": retiring * 100,
                    "speculation_slot_estimate_pct": speculation * 100,
                    "backend_residual_estimate_pct": (1 - frontend - retiring - speculation) * 100,
                })
    write_csv(args.output / "counter-events.csv", raw)
    write_csv(args.output / "counter-summary.csv", metrics)
    (args.output / "counter-errors.json").write_text(json.dumps(errors, indent=2) + "\n")
    if errors:
        raise SystemExit("Missing/unusable perf events; see counter-errors.json. Valid groups were retained.")


if __name__ == "__main__":
    main()
