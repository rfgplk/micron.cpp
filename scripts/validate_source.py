import os

source_exts = { '.cpp', '.hpp' }
RED = "\033[31m"
RESET = "\033[0m"

includes = []
for root, dirs, files in os.walk(".", followlinks=False):
    for file in files:
        _, ext = os.path.splitext(file)
        if ext not in source_exts and ext:
            path = os.path.join(root, file)
            print(f"{RED}{path}{RESET}")
