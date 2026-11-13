#!/usr/bin/env python3
"""Build named FFT probes with duck's flags plus optimization/stack diagnostics."""
import argparse
import json
from pathlib import Path
import re
import shlex
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--compiler", choices=("gcc", "clang"), default="gcc")
    parser.add_argument("--arch", choices=("avx2", "base", "arm", "arm64"), default="avx2")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    target = ["--isa", "v3" if args.arch == "avx2" else "base"] if args.arch in ("avx2", "base") else ["--" + args.arch]
    duck = ["duck", "splat", "build", "tests/build/fft_codegen.cpp", "--" + args.compiler, "--std", "c++23",
            "--perf", "--fp", "--no-ssp", "--no-lto", "--asm", "-i", ".", "-i", "./src", "-o", str(args.output)] + target
    command = shlex.split(subprocess.check_output(duck, text=True).strip())
    command += ["-fstack-usage"]
    if args.compiler == "gcc":
        command += ["-fopt-info-vec-all=" + str(args.output / "vectorization.txt")]
    else:
        command += ["-Rpass=loop-vectorize|slp-vectorizer", "-Rpass-missed=loop-vectorize|slp-vectorizer",
                    "-Rpass-analysis=loop-vectorize|slp-vectorizer"]
    (args.output / "command.json").write_text(json.dumps(command, indent=2) + "\n")
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (args.output / "compiler.stdout").write_text(result.stdout)
    (args.output / "compiler.stderr").write_text(result.stderr)
    result.check_returncode()
    assembly = (args.output / "fft_codegen.s").read_text()
    if args.arch != "avx2":
        return
    # Select the radix-4 f64 body (or its wrapper when Clang inlines it).
    match = re.search(r"^(_ZN\S*__stageILm4ELb0E\S*dEE\S*):.*?^\s*\.size[^\n]*", assembly, re.M | re.S)
    if match is None:
        match = re.search(r"^fft_stage4_f64:.*?^\s*\.size[^\n]*", assembly, re.M | re.S)
    if match is None:
        raise RuntimeError("Cannot locate radix-4 f64 body")
    body = match.group(0)
    (args.output / "stage4_f64.s").write_text(body + "\n")
    lines = body.splitlines()
    loops = []
    for start, line in enumerate(lines):
        label = re.match(r"(\.L[^:]+):", line)
        if label:
            for end in range(start + 1, len(lines)):
                if re.search(r"\bj\w+\s+" + re.escape(label[1]) + r"\b", lines[end]):
                    loop = lines[start:end + 1]
                    loops.append((sum("ymm" in s for s in loop), loop))
                    break
    if not loops:
        raise RuntimeError("Cannot locate vector loop")
    loop = max(loops, key=lambda item: item[0])[1]
    text = "\n".join(loop) + "\n"
    (args.output / "stage4_f64_loop.s").write_text(text)
    result = subprocess.run(["llvm-mca", "-mcpu=haswell", "-iterations=100", "-bottleneck-analysis"], input=text,
                            text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (args.output / "mca.txt").write_text(result.stdout)
    (args.output / "mca.stderr").write_text(result.stderr)
    result.check_returncode()


if __name__ == "__main__":
    main()
