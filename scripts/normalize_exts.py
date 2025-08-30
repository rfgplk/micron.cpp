#!/usr/bin/env python3
import os
import re

src_dir = "src"
tests_dir = "tests"
inc_pattern = re.compile(r'(#include\s*")([^"]+)(\.[^"]+)(")')

def change_extensions_includes(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    new_content = inc_pattern.sub(r'\1\2.hpp\4', content)
    if new_content != content:
        with open(file_path, "w", encoding="utf-8") as f:
            f.write(new_content)

def main():
    for root, _, files in os.walk(src_dir):
        for name in files:
            old_path = os.path.join(root, name)
            base, ext = os.path.splitext(name)
            if ext == ".cpp" or ext == ".cc" or ext == ".asm" or ext == ".s":
                change_extensions_includes(old_path)
                continue
            new_name = base + ".hpp"
            new_path = os.path.join(root, new_name)

            if ext != ".hpp":
                os.rename(old_path, new_path)
            else:
                new_path = old_path

            change_extensions_includes(new_path)
    for root, _, files in os.walk(tests_dir):
        for name in files:
            old_path = os.path.join(root, name)
            base, ext = os.path.splitext(name)
            new_name = base + ".hpp"
            new_path = os.path.join(root, new_name)

            if ext == ".cpp" or ext == ".cc" or ext == ".asm" or ext == ".s":
                change_extensions_includes(old_path)
                continue
            if ext != ".hpp":
                os.rename(old_path, new_path)
            else:
                new_path = old_path

            change_extensions_includes(new_path)


if __name__ == "__main__":
    main()

