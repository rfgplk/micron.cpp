#!/usr/bin/env python3
"""Import the pinned bbmalloc snapshot without changing its upstream checkout."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parent.parent
DEST = ROOT / "src/memory/allocation/wasmalloc/upstream"
REVISION = "224ecfd62df165b5bf39cad02773f940a0bbbfae"
INCLUDE = re.compile(r'^(\s*#\s*include\s*)<micron/([^>]+)>(.*)$', re.MULTILINE)


def git(repo, *args):
    return subprocess.check_output(["git", "-C", str(repo), *args])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, default=Path("/code/C++/bbmalloc"))
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--write", action="store_true")
    action.add_argument("--check", action="store_true")
    args = parser.parse_args()
    files = {}
    hashes = {}
    names = git(args.upstream, "ls-tree", "--name-only", f"{REVISION}:src").decode().splitlines()
    for name in sorted(n for n in names if n.endswith(".hpp")):
        original = git(args.upstream, "show", f"{REVISION}:src/{name}")
        converted = INCLUDE.sub(r'\1"../../../../\2"\3', original.decode()).encode()
        files[name] = converted
        hashes[name] = {
            "source_sha256": hashlib.sha256(original).hexdigest(),
            "generated_sha256": hashlib.sha256(converted).hexdigest(),
        }
    files["manifest.json"] = (json.dumps({"upstream": "bbmalloc", "revision": REVISION,
        "transformation": '<micron/X> includes become "../../../../X"', "headers": hashes},
        indent=2, sort_keys=True) + "\n").encode()
    files["README"] = (
        "GENERATED -- DO NOT EDIT.\n\n"
        f"Pinned bbmalloc revision: {REVISION}\n"
        "Only Micron include paths are rewritten. Refresh or verify with:\n"
        "  python3 scripts/sync_wasmalloc.py --write\n"
        "  python3 scripts/sync_wasmalloc.py --check\n"
        "The script reads Git objects and never modifies the upstream checkout.\n"
    ).encode()
    if args.write:
        DEST.mkdir(parents=True, exist_ok=True)
        for name, content in files.items():
            (DEST / name).write_bytes(content)
    wrong = [name for name, content in files.items()
             if not (DEST / name).exists() or (DEST / name).read_bytes() != content]
    extra = [p.name for p in DEST.iterdir() if p.is_file() and p.name not in files] if DEST.exists() else []
    if wrong or extra:
        print("wasmalloc snapshot differs:", ", ".join(wrong + extra))
        return 1
    print(f"wasmalloc: verified {len(hashes)} headers at {REVISION}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
