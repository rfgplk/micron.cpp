import os

source_exts = {'.c', '.cpp', '.h', '.hpp'}
RED = "\033[31m"
RESET = "\033[0m"

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
                        if not stripped.startswith("//") and ("std::" in stripped or " ::" in stripped) and not "std::initializer_list" in stripped: #checking for both global namespace too
                            print(f"{RED}{path}{RESET}: {stripped}{RESET}")
            except Exception:
                pass
