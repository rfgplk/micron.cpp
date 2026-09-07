#!/usr/bin/env python3
#
# How much of linux/ can a keep-set umbrella still reach?
#
# The barebones port (BAREBONES.md) is severing the include edges that put a syscall behind every
# container. This records the reachability so a regression is visible, and so Phase 4 can tighten
# EXPECTED down to zero instead of re-deriving the numbers by hand.
#
#   scripts/check_syscall_reach.py            # check against EXPECTED, exit 1 on drift
#   scripts/check_syscall_reach.py --print    # print the current table, exit 0
#   scripts/check_syscall_reach.py --list     # also name every reachable header
#
# TWO WAYS THIS MEASUREMENT LIES, BOTH HIT FOR REAL:
#
#  1. `g++ -H` prints the path it walked, not the path on disk:
#     ./src/algorithm/../memory/../linux/sys/types.hpp. A grep for 'src/linux/' matches nothing and
#     reports a clean zero. Every path is normalized before it is counted.
#
#  2. A header that does not exist is a `fatal error`, the include trace stops, and the count is
#     zero for a reason that has nothing to do with syscalls. Measured: `maps/maps.hpp` reported 0
#     because the umbrella is src/maps.hpp. A non-zero compiler exit is a hard failure here, never
#     a result.

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CXX = os.environ.get("CXX", "g++")
STD = os.environ.get("MICRON_STD", "c++26")

# header -> reachable src/port/backends/ headers. measured 2026-09-06 after the Phase 4 prune.
#
# PHASE 4 INVERTED THIS TABLE. It used to count reach into src/linux/, and the goal was to drive
# those numbers to zero. src/linux/ is now deleted, which makes that predicate match nothing and
# every count read 0 -- a gate that passes forever. So it now counts reach into the OS SEAM, and
# src/linux/ is a hard assertion instead (any hit, or the directory existing, is a failure).
#
# what each number means now:
#   0  the header does not touch the operating system AT ALL. concepts/numerics/tuple/algorithm
#      reach this; before Phase 4 every one of them pulled a syscall table.
#   3  a single port facet: <facet>_linux.hpp + __syscall.hpp + syscall_<arch>.hpp
#   4  sort/hash -- the panic facet, via except.hpp -> __exceptions.hpp -> exit.hpp
#   6-8 the containers and the port umbrella: several facets, because the allocator reaches pages
#      and every lock reaches yield and wait
#
# These are EXPECTED and CORRECT. The seam is where the OS is allowed to be.
#
# NOTE, corrected at Phase 5: this used to say that selecting the kernel or metal backend "swaps
# which files answer, not how many". That is false. A kernel facet includes no __syscall.hpp and no
# syscall_<arch>.hpp -- it reaches backends/<facet>_kernel.hpp and backends/__kport_abi.hpp -- so
# the counts change, and the numbers below are the LINUX backend's. Measuring the kernel backend
# wants its own table; the gate that actually gets pointed at it is
# scripts/check_kernel_object.sh, which asserts on the emitted object rather than on includes.
EXPECTED = {
    "concepts.hpp": 0,
    "numerics.hpp": 0,
    "tuple.hpp": 0,
    "algorithm/algorithm.hpp": 0,
    "sort/sort.hpp": 4,
    "hash/hash.hpp": 4,
    "array.hpp": 6,
    "maps.hpp": 8,
    "trees.hpp": 8,
    "lz.hpp": 8,
    "regex.hpp": 8,
    "vector/vector.hpp": 8,
    "strings.hpp": 8,
    "port/panic.hpp": 3,
    "port/yield.hpp": 3,
    "port/ident.hpp": 3,
    "port/clock.hpp": 3,
    "port/pages.hpp": 3,
    # added Phase 5: these two facets were missing from the table entirely, which meant nothing
    # measured them -- port/wait.hpp arrived at Phase 4 and rawmap.hpp with it.
    "port/wait.hpp": 3,
    "port/rawmap.hpp": 3,
    # 8 -> 9 at Phase 5: port.hpp did not include rawmap.hpp, and its banner still claimed "all five
    # facets" when there were seven. Adding the missing include is the drift.
    "port/port.hpp": 9,
}


def reachable(header):
    """Every src/linux/ header pulled in by `#include "<header>"`, normalized."""
    probe = ROOT / "bin" / ".syscall_reach_probe.cpp"
    probe.parent.mkdir(parents=True, exist_ok=True)
    probe.write_text('#include "%s"\nint main(){return 1;}\n' % header)
    try:
        r = subprocess.run(
            [CXX, "-std=" + STD, "-fsyntax-only", "-I./src", "-H", str(probe)],
            cwd=ROOT, capture_output=True, text=True)
    finally:
        probe.unlink(missing_ok=True)

    if r.returncode != 0:
        raise RuntimeError(
            "%s did not compile -- the count would be a false zero, not a result.\n%s"
            % (header, "\n".join(r.stderr.splitlines()[:12])))

    seen = set()
    for line in r.stderr.splitlines():
        # `-H` marks depth with leading dots, then the path it walked
        p = line.lstrip(".").strip()
        if p.startswith("./src/"):
            seen.add(os.path.normpath(p))
    # PHASE 4 INVERSION. Before the prune this measured reach into src/linux/. That directory no
    # longer exists, so the old predicate matches nothing and every count reads 0 -- a gate that
    # cannot fail, which is worse than no gate. It now measures reach into the OS seam itself, and
    # keeps src/linux/ as a HARD ASSERTION rather than a measurement.
    intruders = sorted(p for p in seen if p.startswith("src/linux/"))
    if intruders:
        raise RuntimeError(f"src/linux/ is deleted, yet {header} reached: {intruders}")
    return sorted(p for p in seen if p.startswith("src/port/backends/"))


def main():
    show = "--list" in sys.argv
    printing = "--print" in sys.argv
    bad = 0

    # a directory that must not come back
    if (ROOT / "src" / "linux").exists():
        print("FATAL  src/linux/ exists again -- Phase 4 deleted it.", file=sys.stderr)
        return 1

    # AN ALL-ZERO TABLE IS A BROKEN MEASUREMENT, NOT A RESULT. This is exactly what the old
    # src/linux/ predicate degenerated to the moment the directory was deleted: every count 0,
    # every check passing, forever. If nothing anywhere reaches the OS seam, the probe is broken.
    if not printing and not any(EXPECTED.values()):
        print("FATAL  EXPECTED is all zeros -- the probe measures nothing.", file=sys.stderr)
        return 1

    for header, want in EXPECTED.items():
        try:
            hit = reachable(header)
        except RuntimeError as e:
            print("FATAL  %s" % e)
            bad += 1
            continue

        got = len(hit)
        if printing:
            print("%-26s %d" % (header, got))
        elif got != want:
            print("DRIFT  %-26s expected %d, reachable %d" % (header, want, got))
            for p in hit:
                print("           %s" % p)
            bad += 1
        if show:
            for p in hit:
                print("           %s" % p)

    if printing:
        return 0
    if bad:
        print("\n%d header(s) drifted. If the change is intended, update EXPECTED." % bad)
        return 1
    print("OS-seam (port/backends/) reachability unchanged across %d keep-set umbrellas." % len(EXPECTED))
    return 0


if __name__ == "__main__":
    sys.exit(main())
