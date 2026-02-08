#!/usr/bin/env python3

import os
import re

GREEN = "\033[32m"
RED   = "\033[31m"
RESET = "\033[0m"

path = os.path.join("src", "bits", "__syscall_codes_amd64.hpp")
pattern = re.compile(r"\b\w*SYS_\w*\b")

with open(path, "r", encoding="utf-8", errors="ignore") as f:
    for lineno, line in enumerate(f, 1):
        for match in pattern.findall(line):
            print(f"{RED}{lineno}{RESET}: {GREEN}{match}{RESET}")
