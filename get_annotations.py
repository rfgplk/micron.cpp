import os
import sys

source_exts = {'.c', '.cpp', '.h', '.hpp'}
RED = "\033[31m"
RESET = "\033[0m"

def get_anno(str):
    includes = []
    for root, dirs, files in os.walk("src/", followlinks=False):
        for file in files:
            _, ext = os.path.splitext(file)
            if ext in source_exts:
                path = os.path.join(root, file)
                try:
                    with open(path, "r", encoding="utf-8") as f:
                        for line in f:
                            stripped = line.strip()
                            if str in stripped:
                                print(f"{RED}{path}{RESET}: {stripped}{RESET}")
                except Exception:
                    pass


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("[command] [name_of_annotation] {TODO/NOTE/FIXME/BUG/HACK}")
        exit()
    annos = [ "NOTE", "TODO", "FIXME", "BUG", "HACK"]
    if sys.argv[1] not in annos:
        print("Unknown annotation " + sys.argv[1])
        exit()
    get_anno(sys.argv[1] + ":")
