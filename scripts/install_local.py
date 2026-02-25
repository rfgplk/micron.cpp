#!/usr/bin/env python3
import os
import shutil
import sys

src_path = "./src"
default_dest_dir = "/usr/include/micron"

def main():
    dest = sys.argv[1] if len(sys.argv) > 1 else default_dest_dir

    if not os.path.isdir(dest):
        print("location doesnt exist", file=sys.stderr)
        sys.exit(1)

    if os.geteuid() != 0:
        print("Must be root", file=sys.stderr)
        sys.exit(1)

    for root, _, files in os.walk(src_path):
        for f in files:
            if (
                f.endswith((".h", ".hh", ".hpp", ".c", ".cc", ".cpp"))
                or f in ("initializer_list", "index_sequence", "pthread")
            ):
                src_file = os.path.join(root, f)
                rel_path = os.path.relpath(src_file, src_path)
                dest_file = os.path.join(dest, rel_path)
                os.makedirs(os.path.dirname(dest_file), exist_ok=True)
                shutil.copy2(src_file, dest_file)
                os.chmod(dest_file, 0o644)

if __name__ == "__main__":
    main()
