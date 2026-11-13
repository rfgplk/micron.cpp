#!/usr/bin/env python3
"""Compile the recorded Micron header boundary and optional read-only Canvas probes."""

import argparse
import concurrent.futures
import json
import pathlib
import re
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
INVENTORY = ROOT / "tests/wasm/header_inventory.json"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="clang++-22")
    parser.add_argument("--canvas", type=pathlib.Path, help="Probe this Canvas checkout without writing to it")
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--only", default="", help="Restrict header names by a regular expression")
    parser.add_argument("--record", action="store_true", help="Replace selected inventory observations after reviewing changes")
    parser.add_argument("--output", type=pathlib.Path, default=ROOT / "bin/wasm-headers/results.json")
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    inventory = json.loads(INVENTORY.read_text())
    selected = [entry for entry in inventory["headers"]
                if re.search(args.only, entry["header"]) and (entry["project"] == "micron" or args.canvas)]
    if not selected:
        parser.error("no headers selected")
    canvas = args.canvas.resolve() if args.canvas else None
    if canvas and not (canvas / "src/types.hpp").is_file():
        parser.error("--canvas must name the Canvas checkout root")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    common = [args.cxx, "--no-default-config", "--target=wasm32-unknown-unknown", "-std=c++23",
              "-fhosted", "-fno-builtin", "-D__micron_freestanding=1", "-nostdinc", "-nostdinc++",
              "-fwasm-exceptions", "-mllvm", "-wasm-use-legacy-eh=false", "-mbulk-memory",
              "-I", str(ROOT), "-I", str(ROOT / "src"), "-ferror-limit=8", "-fsyntax-only", "-x", "c++", "-"]

    def probe(item):
        entry, profile = item
        header = entry["header"]
        source = f"#include <micron/{header}>\n" if entry["project"] == "micron" else f'#include "{canvas / "src" / header}"\n'
        flags = ["-msimd128"] if profile == "simd128" else ["-mno-simd128"]
        if entry["project"] == "canvas":
            flags.append("-DCANVAS_NULL=1")
        result = subprocess.run(common[:-1] + flags + ["-"], input=source, capture_output=True, text=True, cwd=ROOT, timeout=60)
        status = "pass" if result.returncode == 0 else "blocked"
        diagnostics = [line.replace(str(ROOT) + "/", "").replace(str(canvas) + "/", "canvas/")
                       for line in result.stderr.splitlines() if "error:" in line][:8]
        log = args.output.parent / f'{entry["project"]}-{profile}-{header.replace("/", "_")}.log'
        log.write_text(result.stderr)
        return {"project": entry["project"], "header": header, "profile": profile, "status": status,
                "diagnostics": diagnostics, "expected": entry.get("status", {}).get(profile),
                "boundary_matched": not entry.get("boundary_message") or entry["boundary_message"] in result.stderr}

    jobs = [(entry, profile) for entry in selected for profile in inventory["profiles"]]
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        results = list(executor.map(probe, jobs))
    changed = [result for result in results if result["status"] != result["expected"]
               or (result["status"] == "blocked" and not result["boundary_matched"])]
    args.output.write_text(json.dumps({"compiler": args.cxx, "results": results}, indent=2) + "\n")
    if args.record:
        by_key = {(entry["project"], entry["header"]): entry for entry in inventory["headers"]}
        for result in results:
            entry = by_key[result["project"], result["header"]]
            entry.setdefault("status", {})[result["profile"]] = result["status"]
            entry.setdefault("diagnostics", {})[result["profile"]] = result["diagnostics"]
        INVENTORY.write_text(json.dumps(inventory, indent=2) + "\n")
    for result in changed:
        print(f'{result["project"]} {result["header"]} {result["profile"]}: {result["expected"]} -> {result["status"]}')
    passed = sum(result["status"] == "pass" for result in results)
    print(f'{passed}/{len(results)} header/profile cells compile; {len(results) - passed} recorded boundaries; {len(changed)} changed observations')
    return 0 if args.record or not changed else 1


if __name__ == "__main__":
    sys.exit(main())
