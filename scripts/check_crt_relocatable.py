#!/usr/bin/env python3
"""THE CRT MUST BE RELOCATABLE, AND TWICE NOW IT WAS NOT.

scripts/install_start.py copies start/ to /usr/src/mc_start (or wherever), and duck reads it back
through --start <dir>.  That only works if every include a crt source issues either

  * stays INSIDE the crt directory -- "__auxv.hpp", "../__crt.hpp" -- so the copy carries it, or
  * goes through the include path -- <micron/...> -- so -i resolves it.

A relative include that climbs OUT of the crt, "../../src/port/init.hpp", resolves in the repo and
nowhere else.  Both failure modes have now been seen in one afternoon:

  start/metal/*.cpp        "../../src/port/init.hpp"      -> /usr/src/src/... , three fatal errors
  start/start.cpp          <micron/attach/mx_entry.hpp>    -> src/attach/ was pruned; on a box with
                                                              an installed snapshot this did NOT
                                                              fail, it resolved to the STALE copy
                                                              and gave 789 redefinition errors

The second is why this checks the angle form too, and why the check is `does it resolve in-tree`
rather than `does it look wrong`.  CLAUDE.md 7 is explicit that a grep is not sound for include
questions; this normalises every path and asks the filesystem.

usage:  python3 scripts/check_crt_relocatable.py [crt_dir] [src_dir]
exit:   0 clean, 1 findings
"""
import os
import re
import sys

INCLUDE = re.compile(r'^\s*#\s*include\s*([<"])([^">]+)[">]')
# a crt source may name these without them existing in-tree: they are the compiler's, not micron's
SYSTEM_OK = ("stdint.h", "stddef.h", "stdarg.h", "float.h", "limits.h")


def main() -> int:
    crt = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "start")
    src = os.path.abspath(sys.argv[2] if len(sys.argv) > 2 else "src")
    if not os.path.isdir(crt):
        print(f"no such crt dir: {crt}", file=sys.stderr)
        return 2

    escapes: list[str] = []
    unresolved: list[str] = []
    checked = 0

    for root, _, files in os.walk(crt):
        for f in sorted(files):
            if not f.endswith((".c", ".cc", ".cpp", ".h", ".hh", ".hpp")):
                continue
            path = os.path.join(root, f)
            rel = os.path.relpath(path, crt)
            with open(path, encoding="utf-8", errors="replace") as fh:
                for n, line in enumerate(fh, 1):
                    m = INCLUDE.match(line)
                    if not m:
                        continue
                    checked += 1
                    kind, target = m.group(1), m.group(2)

                    if kind == '"':
                        # must normalise to something still under the crt root
                        resolved = os.path.normpath(os.path.join(root, target))
                        if os.path.commonpath([resolved, crt]) != crt:
                            escapes.append(f"{rel}:{n}: \"{target}\" leaves the crt -> {resolved}")
                        elif not os.path.exists(resolved):
                            unresolved.append(f"{rel}:{n}: \"{target}\" does not exist")
                    else:
                        # <micron/...> must name a header that is actually in src/, or it will
                        # silently resolve against an installed snapshot instead
                        if target.startswith("micron/"):
                            inside = os.path.join(src, target[len("micron/"):])
                            if not os.path.exists(inside):
                                unresolved.append(
                                    f"{rel}:{n}: <{target}> is not in {os.path.relpath(src)}/ "
                                    f"-- it will resolve against an installed snapshot")
                        elif target not in SYSTEM_OK and "/" not in target:
                            pass  # a bare system header; the compiler's problem, not ours

    for e in escapes:
        print(f"  FAIL  {e}")
    for u in unresolved:
        print(f"  FAIL  {u}")
    bad = len(escapes) + len(unresolved)
    print(f"  {checked} includes checked in {os.path.relpath(crt)}/, {bad} findings")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
