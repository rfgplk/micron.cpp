#!/usr/bin/env python3
"""Generate the Clang compile matrix from the authoritative GCC matrix."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "verify_compile_gcc.duck"
OUTPUT = ROOT / "verify_compile_clang.duck"


def generate() -> str:
    lines = SOURCE.read_text().splitlines()
    result: list[str] = []
    skipping_reflection = False

    for line in lines:
        if line == "# c++26 reflection (-freflection)":
            skipping_reflection = True
            result.extend(
                (
                    "# Clang has no -freflection spelling compatible with micron's GCC reflection gate.",
                    "# Reflection-off coverage remains part of every directory cell above.",
                    "",
                )
            )
            continue
        if skipping_reflection:
            if line == "# chrono":
                skipping_reflection = False
                result.extend(("# %%%%%%%%%%%%%%", "# chrono"))
            continue

        if line.startswith("#  duck verify_compile_gcc.duck"):
            line = "#  duck verify_compile_clang.duck"
        elif line.startswith("#  duck batch parallel verify_compile_gcc.duck"):
            line = "#  duck batch parallel verify_compile_clang.duck   (pool every cell concurrently)"
        elif line.startswith(("build ", "compile ", "link ")):
            command, rest = line.split(" ", 1)
            line = f"{command} --clang {rest}"
            if "--raw-obj" not in line and " -i . " not in line:
                line = line.replace(" -o ", " -i . -o ", 1)

        result.append(line.replace("bin/compiletests/", "bin/compiletests-clang/"))

    return "\n".join(result) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = generate()

    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text() != generated:
            print(f"stale Clang compile matrix: {OUTPUT}", file=sys.stderr)
            return 1
        return 0

    OUTPUT.write_text(generated)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
