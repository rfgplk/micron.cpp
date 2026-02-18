#!/usr/bin/env python3
import os
import shutil
import sys

SRC = "./src"
DEST = "/usr/include/micron"

if os.geteuid() != 0:
    print("Must be root", file=sys.stderr)
    sys.exit(1)

os.makedirs(DEST, exist_ok=True)

for root, _, files in os.walk(SRC):
    for f in files:
        if f.endswith((".h", ".hh", ".hpp", ".c", ".cc", ".cpp")) or f == "initializer_list" or f == "index_sequence" or f == "pthread":
            src_file = os.path.join(root, f)
            rel_path = os.path.relpath(src_file, SRC)
            dest_file = os.path.join(DEST, rel_path)
            os.makedirs(os.path.dirname(dest_file), exist_ok=True)
            shutil.copy2(src_file, dest_file)
            os.chmod(dest_file, 0o644)
