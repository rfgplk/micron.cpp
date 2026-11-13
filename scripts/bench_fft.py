#!/usr/bin/env python3
"""Run FFT measurements sequentially; preserve commands, hardware and raw perf output."""
import argparse
import csv
import json
import os
from pathlib import Path
import subprocess
import time


def capture(command, **kwargs):
    return subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--cpu", type=int, default=2)
    parser.add_argument("--suite", choices=("quick", "full", "none"), default="full")
    parser.add_argument("--perf", action="store_true")
    parser.add_argument("--c2c", action="store_true")
    parser.add_argument("--kernels", action="store_true", help="include the butterfly stage probes")
    parser.add_argument("--cpus", default=None, help="comma-separated physical CPU list for parallel cases")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    binary = str(args.binary.resolve())
    log = []
    metadata = {"binary": binary, "cpu": args.cpu, "time": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
                "affinity": sorted(os.sched_getaffinity(0))}
    physical = {}
    for cpu in sorted(os.sched_getaffinity(0)):
        topology = Path(f"/sys/devices/system/cpu/cpu{cpu}/topology")
        key = ((topology / "physical_package_id").read_text().strip(), (topology / "core_id").read_text().strip())
        physical.setdefault(key, cpu)
    parallel_cpus = list(map(int, args.cpus.split(","))) if args.cpus else list(physical.values())
    if args.cpu in parallel_cpus:
        parallel_cpus.remove(args.cpu)
        parallel_cpus.insert(0, args.cpu)
    metadata["parallel_cpus"] = parallel_cpus
    metadata["processes"] = capture(["ps", "-e", "-o", "pid,psr,comm"]).stdout
    for key, command in (("lscpu", ["lscpu"]), ("kernel", ["uname", "-a"]),
                         ("gcc", ["g++", "--version"]), ("clang", ["clang++", "--version"]),
                         ("sha256", ["sha256sum", binary]), ("git", ["git", "rev-parse", "HEAD"])):
        result = capture(command)
        metadata[key] = result.stdout + result.stderr
    for name in ("perf_event_paranoid", "nmi_watchdog"):
        metadata[name] = Path("/proc/sys/kernel", name).read_text().strip()
    for name in ("scaling_governor", "scaling_cur_freq"):
        path = Path(f"/sys/devices/system/cpu/cpu{args.cpu}/cpufreq", name)
        metadata[name] = path.read_text().strip() if path.exists() else None
    (args.output / "environment.json").write_text(json.dumps(metadata, indent=2) + "\n")

    def run(command, stem=None, timeout=180):
        result = capture(command, timeout=timeout)
        log.append({"argv": command, "returncode": result.returncode})
        (args.output / "commands.json").write_text(json.dumps(log, indent=2) + "\n")
        if stem:
            (args.output / (stem + ".stdout")).write_text(result.stdout)
            (args.output / (stem + ".stderr")).write_text(result.stderr)
        return result

    cases = []
    if args.suite != "none":
        lengths = [8, 16, 32, 64, 256, 1000, 1009, 1024, 4096, 16384]
        if args.suite == "full":
            lengths += [3, 5, 7, 60, 105, 125, 343, 1536, 4093, 65536, 262144, 1048576]
        for precision in ("f32", "f64"):
            cases += [("c2c", precision, n, 0, 1) for n in lengths]
            for n in ([64, 1024, 16384] if args.suite == "full" else [1024]):
                cases += [(mode, precision, n, 0, 1) for mode in
                          ("inverse", "inplace_pair", "engine", "multiply", "copy", "gather", "strided",
                           "split", "r2c", "c2r", "dct", "convolve", "plan", "workspace")]
            cases += [("batch", precision, n, 0, 1) for n in (16, 64, 1024)]
            cases += [("nd", precision, n, 0, 1) for n in (32, 128)]
            cases += [("c2c", precision, 4096, offset, 1) for offset in (1, 2, 3)]
            if args.suite == "full":
                cases += [("multiply", precision, n, off, 1) for n in (7, 31, 255, 4096, 262144) for off in (0, 1)]
        # A CPU set is used below for actual parallel work; one-CPU affinity serializes all workers.
        cases += [("parallel", "f64", n, 0, workers) for n in (1024, 16384, 262144)
                  for workers in (1, 2, 4) if workers <= len(parallel_cpus)]
    if args.kernels:
        cases += [(f"stage{radix}", precision, n, offset, 1) for radix in (2, 3, 4, 5, 7)
                  for precision in ("f32", "f64") for n in (1, 2, 3, 4, 7, 8, 15, 16, 31, 64, 128, 1024, 8192)
                  for offset in (0, 1)]
    rows = []
    for mode, precision, n, offset, workers in cases:
        cpus = str(args.cpu) if workers == 1 else ",".join(map(str, parallel_cpus[:workers]))
        command = ["taskset", "-c", cpus, binary, mode, precision, str(n), "0", "7", str(offset), str(workers)]
        result = run(command)
        if result.returncode:
            raise RuntimeError(f"{command}: {result.stderr}\n{result.stdout}")
        data = list(csv.DictReader(result.stdout.splitlines()))
        if len(data) != 1:
            raise RuntimeError(f"Expected one CSV row: {result.stdout}")
        rows += data
    if rows:
        with (args.output / "timings.csv").open("w") as f:
            writer = csv.DictWriter(f, rows[0].keys())
            writer.writeheader()
            writer.writerows(rows)

    if args.perf:
        # Haswell: at most three programmable events per group (NMI watchdog may occupy one).
        # Delivery, issue and retirement share a group; only recovery is sampled separately.
        groups = {
            "slots": "cycles:u,instructions:u,idq_uops_not_delivered.core:u,uops_issued.any:u,uops_retired.retire_slots:u",
            "retire": "cycles:u,uops_retired.retire_slots:u,int_misc.recovery_cycles:u",
            "stalls": "cycles:u,resource_stalls.any:u,resource_stalls.rs:u,resource_stalls.sb:u",
            "cache": "cycles:u,cycle_activity.stalls_l1d_pending:u,cycle_activity.stalls_l2_pending:u,cycle_activity.stalls_ldm_pending:u",
            "loads": "cycles:u,mem_uops_retired.all_loads:u",
            "stores": "cycles:u,mem_uops_retired.all_stores:u",
            "l1miss": "cycles:u,mem_load_uops_retired.l1_miss:u",
            "ports015": "cycles:u,uops_executed_port.port_0:u,uops_executed_port.port_1:u,uops_executed_port.port_5:u",
            "ports234": "cycles:u,uops_executed_port.port_2:u,uops_executed_port.port_3:u,uops_executed_port.port_4:u",
            "branches": "cycles:u,instructions:u,branches:u,branch-misses:u",
            "forwarding": "cycles:u,ld_blocks.store_forward:u",
            "decode": "cycles:u,idq.dsb_uops:u,idq.mite_uops:u,icache.misses:u",
            "microcode": "cycles:u,idq.ms_uops:u,arith.divider_uops:u",
        }
        for mode, precision, n, iterations in (("c2c", "f64", 1024, 100000), ("c2c", "f32", 4096, 40000),
                                               ("multiply", "f64", 4096, 100000), ("strided", "f64", 1024, 30000)):
            for group, events in groups.items():
                stem = f"perf-{mode}-{precision}-{n}-{group}"
                run(["perf", "stat", "-x", ",", "-r", "3", "-o", str(args.output / (stem + ".csv")),
                     "-e", "{" + events + "}", "--", "taskset", "-c", str(args.cpu), binary,
                     mode, precision, str(n), str(iterations), "1", "0", "1"], stem)
    if args.c2c:
        stem = "c2c-parallel-f64-16384"
        data = args.output / (stem + ".data")
        cpus = ",".join(map(str, parallel_cpus[:4]))
        result = run(["perf", "c2c", "record", "-o", str(data), "--", "taskset", "-c", cpus,
                      binary, "parallel", "f64", "16384", "2000", "1", "0", "4"], stem)
        if result.returncode:
            # Physical-address sampling requires privileges that ordinary PEBS does not.
            # Virtual addresses suffice for sharing within this single process.
            result = run(["perf", "c2c", "record", "--no-phys-data", "-o", str(data), "--", "taskset", "-c", cpus,
                          binary, "parallel", "f64", "16384", "2000", "1", "0", "4"], stem + "-virtual")
        if result.returncode == 0:
            run(["perf", "c2c", "report", "--stdio", "-i", str(data)], stem + "-report")
    (args.output / "commands.json").write_text(json.dumps(log, indent=2) + "\n")


if __name__ == "__main__":
    main()
