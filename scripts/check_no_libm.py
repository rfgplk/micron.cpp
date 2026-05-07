#!/usr/bin/env python3

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DIR = ROOT / "src" / "math"

SCAN_SUFFIXES = {".hpp", ".cpp", ".cc", ".cxx", ".c", ".h", ".ipp", ".tpp"}

ALLOW = re.compile(
    r"^__builtin_("
    r"bit_cast|bswap16|bswap32|bswap64|"
    r"clz|clzl|clzll|ctz|ctzl|ctzll|"
    r"popcount|popcountl|popcountll|parity|parityl|parityll|"
    r"rotateleft32|rotateleft64|rotateright32|rotateright64|"
    r"add_overflow|sub_overflow|mul_overflow|umul_overflow|"
    r"isnan|isinf|isinf_sign|isfinite|isnormal|signbit|"
    r"isgreater|isgreaterequal|isless|islessequal|islessgreater|isunordered|"
    r"fma|fmaf|fmal|"
    r"memcpy|prefetch|assoc_barrier|"
    r"huge_val|huge_valf|huge_vall|nan|nanf|nanl|nans|nansf|nansl|"
    r"abs|labs|llabs"
    r")$"
)

# Names that look like __builtin_<x>{,f,l}, only allowed inside the
# if-consteval folding branches of kern.hpp / hw.hpp.
KERN_BUILTINS = re.compile(
    r"^__builtin_("
    r"sin|cos|tan|asin|acos|atan|atan2|sinh|cosh|tanh|asinh|acosh|atanh|"
    r"exp|exp2|expm1|log|log2|log10|log1p|sqrt|cbrt|"
    r"erf|erfc|tgamma|lgamma|j0|j1|y0|y1|"
    r"floor|ceil|trunc|round|rint|nearbyint|"
    r"atan2|pow|hypot|fmod|remainder|copysign"
    r")(f|l)?$"
)

CONSTEVAL_FILES = {"kern.hpp", "hw.hpp"}

LINE_COMMENT = re.compile(r"//.*$", re.MULTILINE)
BUILTIN_REF = re.compile(r"__builtin_[A-Za-z0-9_]+")


def scan_file(path: Path) -> bool:
    """Return True if file is clean, False if it has disallowed builtins."""
    try:
        src = path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return True

    code = LINE_COMMENT.sub("", src)
    refs = sorted(set(BUILTIN_REF.findall(code)))
    if not refs:
        return True

    is_consteval_file = path.name in CONSTEVAL_FILES
    bad = []
    for r in refs:
        if ALLOW.match(r):
            continue
        if is_consteval_file and KERN_BUILTINS.match(r):
            continue
        bad.append(r)

    if bad:
        try:
            display = path.relative_to(ROOT)
        except ValueError:
            display = path
        print(f"FAIL: {display}")
        for r in bad:
            print(f"    {r}")
        return False
    return True


def main() -> int:
    if not DIR.is_dir():
        print(f"FAIL — {DIR} does not exist or is not a directory.")
        return 1

    failed = False
    for f in sorted(DIR.rglob("*")):
        if f.is_file() and f.suffix in SCAN_SUFFIXES:
            if not scan_file(f):
                failed = True

    if failed:
        print()
        print("FAIL: unexpected __builtin_* references in src/math/*")
        print("Either remove them, or extend the allowlist after careful review")
        return 1

    print("OK: every __builtin_* reference under src/math/ is on the LIGHT")
    print("     allowlist (or the kern.hpp / hw.hpp consteval shim)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
