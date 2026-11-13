#!/usr/bin/env python3
# Copyright (c) 2026- David Lucius Severus
# Distributed under the Boost Software License, Version 1.0.
"""Pin and measure only the warmed loop with perf stat's acknowledged controls."""
import argparse
import hashlib
import json
import os
import pathlib
import platform
import statistics
import subprocess

CASES = [f"{op}_{span}" for span in ("common", "wide")
         for op in ("decimal_exact", "decimal_17", "parse_17", "parse_exact")]
CASES += [f"{op}_{size}" for size in (64, 4096, 65536) for op in ("frame", "view")]
CASES += ["scalar_write", "scalar_read", "vector_frame", "vector_unframe", "string_unframe", "nested_unframe", "symbolic_write", "symbolic_write_scratch"]
EVENTS = ["cycles:u", "instructions:u", "branches:u", "branch-misses:u", "cache-misses:u"]


def row(output, case):
    lines = [s for s in output.splitlines() if s.startswith(case + ",")]
    if len(lines) != 1:
        raise RuntimeError(f"expected one {case} CSV row: {output}")
    fields = lines[0].split(",")
    name, operations, size, ns, allocations, checksum = fields[:6]
    return dict(case=name, operations=int(operations), bytes=int(size), elapsed_ns=int(ns),
                cpu_ns=int(fields[6]) if len(fields) > 6 else None,
                tracked_allocations=int(allocations), checksum=int(checksum))


def run(binary, case, repeats, cpu, perf=False):
    command = ["taskset", "-c", str(cpu), binary, case, str(repeats)]
    descriptors = []
    try:
        if perf:
            control_read, control_write = os.pipe()
            ack_read, ack_write = os.pipe()
            descriptors = [control_read, control_write, ack_read, ack_write]
            command += [str(control_write), str(ack_read)]
            command = ["perf", "stat", "--no-big-num", "-x", ";", "-D", "-1", "--control",
                       f"fd:{control_read},{ack_write}", "-e", ",".join(EVENTS), "--"] + command
        process = subprocess.run(command, text=True, capture_output=True, pass_fds=descriptors, timeout=120)
        if process.returncode != 1:
            raise RuntimeError(f"exit={process.returncode}: {command}\n{process.stdout}\n{process.stderr}")
        sample = row(process.stdout, case)
        if perf:
            counters = {}
            for line in process.stderr.splitlines():
                fields = line.split(";")
                if len(fields) >= 3 and fields[2] in EVENTS:
                    counters[fields[2]] = dict(value=float(fields[0]), unit=fields[1], raw=line)
            if set(counters) != set(EVENTS):
                raise RuntimeError(f"missing counters: {process.stderr}")
            sample["perf"] = counters
        return sample
    finally:
        for fd in descriptors:
            os.close(fd)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", action="append", required=True, help="label=path")
    parser.add_argument("--output", required=True)
    parser.add_argument("--samples", type=int, default=5)
    parser.add_argument("--milliseconds", type=int, default=150)
    parser.add_argument("--cpu", type=int, default=min(os.sched_getaffinity(0)))
    parser.add_argument("--cases", nargs="+", default=CASES)
    parser.add_argument("--allocation-scope", default="tracking_allocator<190> container calls; other allocations are not intercepted")
    args = parser.parse_args()
    report = dict(platform=platform.platform(), cpu=platform.processor(), pin=args.cpu,
                  samples=args.samples, target_ms=args.milliseconds, events=EVENTS,
                  pmu_scope="acknowledged enable/disable around warmed timed loop; setup and output excluded",
                  allocation_scope=args.allocation_scope,
                  profiles=[], results=[])
    for spec in args.binary:
        label, path = spec.split("=", 1)
        report["profiles"].append(dict(label=label, binary=path, sha256=hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()))
    for case in args.cases:
        first = args.binary[0].split("=", 1)[1]
        calibration = run(first, case, 4, args.cpu)
        repeats = max(4, min(1000000, int(4 * args.milliseconds * 1000000 / max(1, calibration["elapsed_ns"]))))
        samples = {spec.split("=", 1)[0]: [] for spec in args.binary}
        # Alternate before/after ordering between samples to reduce thermal drift.
        for repeat in range(args.samples):
            order = args.binary if repeat % 2 == 0 else list(reversed(args.binary))
            for spec in order:
                label, path = spec.split("=", 1)
                samples[label].append(run(path, case, repeats, args.cpu, True))
        checksums = {s["checksum"] for values in samples.values() for s in values}
        if len(checksums) != 1:
            raise RuntimeError(f"before/after checksum mismatch for {case}: {checksums}")
        for label, values in samples.items():
            ns = statistics.median(s["elapsed_ns"] / s["operations"] for s in values)
            cpu_ns = statistics.median(s["cpu_ns"] / s["operations"] for s in values) if all(s["cpu_ns"] is not None for s in values) else None
            counters = {event: statistics.median(s["perf"][event]["value"] / s["operations"] for s in values) for event in EVENTS}
            report["results"].append(dict(profile=label, case=case, repeats=repeats,
                                          median_ns_per_operation=ns, median_cpu_ns_per_operation=cpu_ns, median_events_per_operation=counters,
                                          allocations_per_operation=statistics.median(s["tracked_allocations"] / s["operations"] for s in values),
                                          samples=values))
            print(f"{label:12} {case:24} {ns:10.2f} wall ns/op {cpu_ns or 0:10.2f} CPU ns/op {counters['cycles:u']:10.2f} cycles/op", flush=True)
        pathlib.Path(args.output).write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
